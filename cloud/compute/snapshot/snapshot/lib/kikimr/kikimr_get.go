package kikimr

import (
	"database/sql"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

func (st *kikimrstorage) GetLiveSnapshot(ctx context.Context, id string) (info *common.SnapshotInfo, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	info, err = st.GetSnapshot(ctx, id)
	if err != nil {
		return
	}
	if info.State.Code == storage.StateDeleting || info.State.Code == storage.StateDeleted || info.State.Code == storage.StateRogueChunks {
		log.G(ctx).Error("GetLiveSnapshot: snapshot is deleted", zap.Error(misc.ErrSnapshotNotFound))
		return nil, misc.ErrSnapshotNotFound
	}
	return
}

func (st *kikimrstorage) GetSnapshot(ctx context.Context, id string) (info *common.SnapshotInfo, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	t := misc.GetSnapshotTimer.Start()
	defer t.ObserveDuration()
	err = st.retryWithTx(ctx, "GetSnapshotImpl", func(tx Querier) error {
		info, err = st.GetSnapshotImpl(ctx, tx, id)
		return err
	})
	return
}

func (st *kikimrstorage) GetSnapshotInternal(ctx context.Context, id string) (resSnapshot *storage.SnapshotInternal, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var sh storage.SnapshotInternal
	err := st.retryWithTx(ctx, "GetSnapshotInternal", func(tx Querier) error {
		return st.getSnapshotFields(ctx, tx, id, []string{"tree", "checksum", "chunksize"},
			[]interface{}{&sh.Tree, nullString{&sh.Checksum}, &sh.ChunkSize})
	})
	if err != nil {
		return nil, err
	}
	return &sh, nil
}

func (st *kikimrstorage) GetSnapshotFull(ctx context.Context, id string) (resSnapshot *storage.SnapshotFull, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var sh storage.SnapshotFull
	err := st.retryWithTx(ctx, "GetSnapshotFull", func(tx Querier) error {
		err := st.getSnapshotFields(
			ctx, tx, id,
			[]string{"id", "base", "state", "created", "size", "realsize",
				"metadata", "organization", "project", "disk", "public",
				"name", "description", "productid", "imageid",
				"tree", "checksum", "chunksize",
			},
			[]interface{}{&sh.ID, nullString{&sh.Base}, &sh.State.Code, kikimrTime{&sh.Created}, &sh.Size, &sh.RealSize,
				&sh.Metadata, &sh.Organization, &sh.ProjectID, &sh.Disk, &sh.Public,
				&sh.Name, &sh.Description, &sh.ProductID, &sh.ImageID,
				&sh.Tree, nullString{&sh.Checksum}, &sh.ChunkSize,
			})
		if err != nil {
			return err
		}

		// For non-ready snapshots
		if sh.ChunkSize == 0 {
			sh.ChunkSize, err = st.tryGetChunkSize(ctx, tx, sh.Tree, id)
		}
		return err
	})
	if err != nil {
		return nil, err
	}

	return &sh, nil
}

func (st *kikimrstorage) IsSnapshotCleared(ctx context.Context, id string) (cleared bool, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	err = st.retryWithTx(ctx, "IsSnapshotClearedImpl", func(tx Querier) error {
		cleared, err = st.IsSnapshotClearedImpl(ctx, tx, id)
		return err
	})
	return
}

func (st *kikimrstorage) getTableFields(
	ctx context.Context, tx Querier, declare, from, where string, args []interface{},
	fieldNames []string, fieldDst []interface{}) error {

	query := fmt.Sprintf("#PREPARE %s SELECT %s FROM %s WHERE %s",
		declare, strings.Join(fieldNames, ", "), from, where)
	err := tx.QueryRowContext(ctx, query, args...).Scan(fieldDst...)

	switch err {
	case nil:
	case sql.ErrNoRows:
		log.G(ctx).Info("getTableFields failed: absent", zap.Error(misc.ErrSnapshotNotFound))
		return misc.ErrSnapshotNotFound
	default:
		log.G(ctx).Error("getTableFields failed", zap.Error(err), zap.Strings("fields", fieldNames))
		return kikimrError(err)
	}
	return nil
}

func (st *kikimrstorage) getDeltaFields(ctx context.Context, tx Querier, id string, fieldNames []string, fieldDst []interface{}) error {
	return st.getTableFields(ctx, tx, "DECLARE $id AS Utf8;", "$snapshots AS t", "t.id = $id",
		[]interface{}{sql.Named("id", id)}, fieldNames, fieldDst)
}

func (st *kikimrstorage) getSnapshotFields(ctx context.Context, tx Querier, snapshotID string, fieldNames []string, fieldDst []interface{}) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	// TODO: Performance does not depend on how many fields we read.
	// So it may make sense to change all calls to GetSnapshotFull.

	where := "t.id = $id"
	declare := "DECLARE $id AS Utf8; "
	args := []interface{}{sql.Named("id", snapshotID)}

	// Dirty hack for transparent joining of statedescription
	if !stringSliceContains(fieldNames, "statedescription") {
		return st.getTableFields(ctx, tx, declare, "$snapshotsext AS t",
			where, args, fieldNames, fieldDst)
	}

	from := "$snapshotsext AS t LEFT JOIN " +
		"$snapshotsext_statedescription AS sesd ON t.id = sesd.id"
	newFieldNames := make([]string, 0, len(fieldNames))
	for _, field := range fieldNames {
		if field == "statedescription" {
			newFieldNames = append(newFieldNames, "sesd."+field)
		} else {
			newFieldNames = append(newFieldNames, "t."+field)
		}
	}
	return st.getTableFields(ctx, tx, declare, from,
		where, args, newFieldNames, fieldDst)
}

func (st *kikimrstorage) GetSnapshotImpl(ctx context.Context, tx Querier, id string) (snapshotRes *common.SnapshotInfo, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var sh common.SnapshotInfo
	err := st.getSnapshotFields(
		ctx, tx, id,
		[]string{"id", "base", "state", "created", "size", "realsize",
			"metadata", "organization", "project", "disk", "public",
			"name", "description", "productid", "imageid", "statedescription"},
		[]interface{}{&sh.ID, nullString{&sh.Base}, &sh.State.Code, kikimrTime{&sh.Created}, &sh.Size, &sh.RealSize,
			&sh.Metadata, &sh.Organization, &sh.ProjectID, &sh.Disk, &sh.Public,
			&sh.Name, &sh.Description, &sh.ProductID, &sh.ImageID, nullString{&sh.State.Description},
		})
	if err != nil {
		return nil, err
	}

	// TODO: Get  sharing

	rows, err := tx.QueryContext(ctx,
		"SELECT `timestamp`, realsize "+
			"FROM $sizechanges "+
			"WHERE id = $1 ORDER BY `timestamp` ASC", sh.ID)
	if err != nil {
		log.G(ctx).Error("GetSnapshotImpl: querying changes failed", zap.Error(err))
		return nil, kikimrError(err)
	}

	sh.Changes = []common.SizeChange{}
	for rows.Next() {
		var timestamp time.Time
		var realsize int64
		if err := rows.Scan(kikimrTime{&timestamp}, &realsize); err != nil {
			log.G(ctx).Error("GetSnapshotImpl: changes scan failed", zap.Error(err))
			return nil, kikimrError(err)
		}
		sh.Changes = append(sh.Changes, common.SizeChange{
			Timestamp: timestamp,
			RealSize:  realsize,
		})
	}

	return &sh, nil
}

func (st *kikimrstorage) IsSnapshotClearedImpl(ctx context.Context, tx Querier, id string) (res bool, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	err := tx.QueryRowContext(ctx,
		"SELECT id FROM $snapshots WHERE id = $1", id).Scan(&id)
	switch err {
	case nil:
	case sql.ErrNoRows:
		return true, nil
	default:
		log.G(ctx).Error("IsSnapshotClearedImpl: unknown error", zap.Error(err))
		return false, kikimrError(err)
	}
	return false, nil
}
