package kikimr

import (
	"database/sql"
	"fmt"
	"strings"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

func (st *kikimrstorage) UpdateSnapshotStatus(ctx context.Context, id string, status storage.StatusUpdate) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	if status.State != storage.StateCreating && status.State != storage.StateFailed && status.State != storage.StateRogueChunks && status.State != storage.StateDeleting {
		log.G(ctx).Error("UpdateSnapshotStatus: invalid transition", zap.String("new", status.State))
		return xerrors.Errorf("updateSnapshotStatus: invalid transition, status=%v", status)
	}

	t := misc.UpdateSnapshotStatus.Start()
	defer t.ObserveDuration()

	return st.retryWithTx(ctx, "UpdateSnapshotStatusImpl", func(tx Querier) error {
		return st.UpdateSnapshotStatusImpl(ctx, tx, id, status)
	})
}

func (st *kikimrstorage) UpdateSnapshot(ctx context.Context, r *common.UpdateRequest) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	t := misc.UpdateSnapshot.Start()
	defer t.ObserveDuration()
	return st.retryWithTx(ctx, "UpdateSnapshotImpl", func(tx Querier) error {
		return st.UpdateSnapshotImpl(ctx, tx, r)
	})
}

func (st *kikimrstorage) UpdateSnapshotChecksum(ctx context.Context, id, checksum string) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	return st.retryWithTx(ctx, "UpdateSnapshotChecksum", func(tx Querier) error {
		_, err := tx.ExecContext(ctx,
			"#PREPARE "+
				"DECLARE $checksum AS Utf8; "+
				"DECLARE $id AS Utf8; "+
				"UPDATE $snapshotsext SET checksum = $checksum WHERE id = $id",
			sql.Named("checksum", checksum), sql.Named("id", id))
		if err != nil {
			log.G(ctx).Error("UpdateSnapshotChecksum: update front DB failed", zap.Error(err))
			return kikimrError(err)
		}

		return nil
	})
}

func (st *kikimrstorage) UpdateSnapshotStatusImpl(ctx context.Context, tx Querier, id string, status storage.StatusUpdate) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	// Select current state with lock
	var state, desc string
	err := st.getSnapshotFields(ctx, tx, id, []string{"state", "statedescription"},
		[]interface{}{&state, nullString{&desc}})
	if err != nil {
		return err
	}

	log.G(ctx).Debug("UpdateSnapshotStatusImpl: update", zap.String("old", state),
		zap.String("new", status.State), zap.String("desc", status.Desc))

	// Conversion workflow
	if state != storage.StateCreating && state != storage.StateRogueChunks {
		log.G(ctx).Error("UpdateSnapshotStatusImpl: state changed", zap.String("state", state))
		return misc.ErrStateChanged
	}

	if state == status.State && desc >= status.Desc {
		// Nothing to update
		return misc.ErrNoCommit
	}

	_, err = tx.ExecContext(ctx,
		"#PREPARE "+
			"DECLARE $id AS Utf8; "+
			"DECLARE $state AS Utf8; "+
			"DECLARE $failed AS Utf8; "+
			"DECLARE $creating AS Utf8; "+
			"DECLARE $desc AS Utf8; "+
			"$conditional = AsList("+
			"  AsStruct($id AS id, $id AS chain, $state AS state)"+
			");"+
			"UPDATE $snapshotsext ON (id, state) "+
			"  SELECT $id AS id, $state AS state "+
			"  FROM AS_TABLE($conditional) WHERE state != $creating; "+
			"UPSERT INTO $snapshotsext_gc (id, chain) "+
			"  SELECT id, chain "+
			"  FROM AS_TABLE($conditional) WHERE state = $failed; "+
			"UPDATE $snapshotsext_statedescription ON (id, statedescription) "+
			"  VALUES ($id, $desc)",
		sql.Named("id", id),
		sql.Named("state", status.State),
		sql.Named("failed", storage.StateFailed),
		sql.Named("creating", storage.StateCreating),
		sql.Named("desc", status.Desc))
	if err != nil {
		log.G(ctx).Error("UpdateSnapshotStatusImpl: update failed", zap.Error(err))
		return kikimrError(err)
	}

	return nil
}

func (st *kikimrstorage) UpdateSnapshotImpl(ctx context.Context, tx Querier, r *common.UpdateRequest) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var state string
	var public bool
	err := st.getSnapshotFields(ctx, tx, r.ID, []string{"state", "public"}, []interface{}{&state, &public})
	if err != nil {
		return err
	}

	if state == storage.StateDeleting || state == storage.StateDeleted || state == storage.StateRogueChunks {
		log.G(ctx).Error("UpdateSnapshotImpl: snapshot is deleted", zap.String("state", state))
		return misc.ErrSnapshotNotFound
	}

	clauses := make([]string, 0, 3)
	args := []interface{}{r.ID}

	if r.Metadata != nil {
		args = append(args, *r.Metadata)
		clauses = append(clauses, fmt.Sprintf("metadata = $%v", len(args)))
	}
	if r.Description != nil {
		args = append(args, *r.Description)
		clauses = append(clauses, fmt.Sprintf("description = $%v", len(args)))
	}
	if r.Public != nil {
		args = append(args, *r.Public)
		clauses = append(clauses, fmt.Sprintf("public = $%v", len(args)))
	}

	if len(clauses) == 0 {
		log.G(ctx).Info("UpdateSnapshotImpl: nothing to update")
		return nil
	}

	_, err = tx.ExecContext(ctx,
		"UPDATE $snapshotsext SET "+strings.Join(clauses, ",")+" WHERE id = $1",
		args...)
	if err != nil {
		log.G(ctx).Error("UpdateSnapshotImpl: update failed", zap.Error(err))
		return kikimrError(err)
	}

	if r.Public != nil && *r.Public != public {
		if *r.Public {
			_, err = tx.ExecContext(ctx,
				"UPSERT INTO $snapshotsext_public (id) VALUES ($1)", r.ID)
			if err != nil {
				log.G(ctx).Error("UpdateSnapshotImpl: public index in front DB failed", zap.Error(err))
				return kikimrError(err)
			}
		} else {
			_, err = tx.ExecContext(ctx,
				"DELETE FROM $snapshotsext_public WHERE id = $1", r.ID)
			if err != nil {
				log.G(ctx).Error("UpdateSnapshotImpl: delete public index in front DB failed", zap.Error(err))
				return kikimrError(err)
			}
		}
	}

	return nil
}
