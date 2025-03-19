package kikimr

import (
	"database/sql"
	"fmt"
	"sort"
	"strings"
	"sync"
	"time"

	"golang.org/x/xerrors"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"

	"a.yandex-team.ru/cloud/compute/go-common/util"

	"go.uber.org/zap"
	"golang.org/x/net/context"
	"golang.org/x/sync/errgroup"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
)

const childRebaseAttempts = 3

// Delete stages
func (st *kikimrstorage) BeginDeleteSnapshot(ctx context.Context, id string) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	log.G(ctx).Debug("BeginDeleteSnapshot start")
	defer func() { log.G(ctx).Debug("BeginDeleteSnapshot finish", zap.Error(resErr)) }()

	return st.retryWithTx(ctx, "BeginDeleteSnapshotImpl", func(tx Querier) error {
		return st.BeginDeleteSnapshotImpl(ctx, tx, id)
	})
}

func (st *kikimrstorage) clearSnapshotData(ctx context.Context, id string, forceLock bool) (err error) {
	/*
		Algorithm:

		Initialize
		1. Lock snapshot
		2. Check chains for dependent snapshots with earlier deletion time
		3. Select children

		Rebase children (after that, our children will become our siblings)
		1. Lock child
		2. Select difference
		3. Upsert inherited chunks to snapshotchunks, update refcnt
		4. Rebase childr
		5. Unlock child

		Clear data (after that, we will have no chunks)
		1. Decrease refcnt
		2. (optional) Select chunks with refcnt == 1
		3. Remove selected chunks from blobs, chunks, snapshotchunks

		Finalize
		1. Remove snapshot from deltas
		2. Unlock snapshot
	*/

	t := misc.ClearSnapshot.Start(ctx)
	defer t.ObserveDuration()
	return (&clearContext{st: st, id: id}).ClearSnapshotDataImpl(ctx, forceLock)
}

func (st *kikimrstorage) endDeleteSnapshot(ctx context.Context, id string, synchro bool, forceLock bool) error {
	var wg sync.WaitGroup
	wg.Add(1)

	ctx = misc.WithNoCancel(ctx)
	st.wg.Add(1)
	var ctxErr error
	go func() {
		defer st.wg.Done()
		defer wg.Done()
		if err := st.clearSnapshotData(ctx, id, forceLock); err != nil && err != misc.ErrSnapshotNotFound {
			ctxErr = err
			return
		}
		ctxErr = st.retryWithTx(ctx, "endDeleteSnapshotImpl", func(tx Querier) error {
			return st.endDeleteSnapshotImpl(ctx, tx, id)
		})
	}()

	if synchro {
		wg.Wait()
		return ctxErr
	}

	return nil
}

// Async version for server
func (st *kikimrstorage) EndDeleteSnapshot(ctx context.Context, id string, forceLock bool) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	return st.endDeleteSnapshot(ctx, id, false, forceLock)
}

// Sync version for GC
func (st *kikimrstorage) EndDeleteSnapshotSync(ctx context.Context, id string, forceLock bool) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	log.G(ctx).Debug("EndDeleteSnapshotSync start")
	defer func() { log.G(ctx).Debug("EndDeleteSnapshotSync finish", zap.Error(resErr)) }()

	return st.endDeleteSnapshot(ctx, id, true, forceLock)
}

/* Delete snapshot data without changing front DB.
   Unsafe, used for conversion path only.
*/
//nolint:errcheck
func (st *kikimrstorage) ClearSnapshotData(ctx context.Context, id string) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	ctx = misc.WithNoCancel(ctx)
	st.wg.Add(1)
	go func() {
		defer st.wg.Done()
		log.DebugErrorCtx(ctx, st.clearSnapshotData(ctx, id, false), "clearSnapshotData")
	}()
	return nil
}

func (st *kikimrstorage) DeleteTombstone(ctx context.Context, id string) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	return st.retryWithTx(ctx, "DeleteTombstoneImpl", func(tx Querier) error {
		return st.DeleteTombstoneImpl(ctx, tx, id)
	})
}

func (st *kikimrstorage) addToGC(ctx context.Context, tx Querier, id string) error {
	// TODO: Batches?
	rows, err := tx.QueryContext(ctx,
		"#PREPARE "+
			"DECLARE $id AS Utf8;"+
			"$refs = (SELECT id AS tgt, base AS we FROM $snapshots_base"+
			" UNION ALL SELECT base AS tgt, id AS we FROM $snapshots);"+
			"SELECT DISTINCT(segc.chain) FROM $refs as refs"+
			" INNER JOIN $snapshotsext_gc AS segc"+
			" ON refs.tgt = segc.id WHERE refs.we = $id", sql.Named("id", id))
	if err != nil {
		log.G(ctx).Error("addToGC: getting chain ids failed", zap.Error(err))
		return kikimrError(err)
	}
	//nolint:errcheck
	defer rows.Close()

	var chains []interface{}
	for rows.Next() {
		var chain string
		if err = rows.Scan(&chain); err != nil {
			log.G(ctx).Error("addToGC: scanning to chain failed", zap.Error(err))
			return kikimrError(err)
		}
		chains = append(chains, chain)
	}

	if err = rows.Err(); err != nil {
		log.G(ctx).Error("addToGC: rows.Err", zap.Error(err))
		return kikimrError(err)
	}

	var chain string
	switch len(chains) {
	case 0:
		chain = id
	case 1:
		chain = chains[0].(string)
	default:
		q := newQueryBuilder(chains[0])
		q.AppendInBraces(chains[1:]...)
		_, err = tx.ExecContext(ctx,
			"UPDATE $snapshotsext_gc SET chain = $1 WHERE chain in "+strings.Join(q.clauses, ","), q.args...)
		if err != nil {
			log.G(ctx).Error("addToGC: chain merging failed", zap.Error(err))
			return kikimrError(err)
		}
		chain = chains[0].(string)
	}

	_, err = tx.ExecContext(ctx,
		"#PREPARE "+
			"DECLARE $id AS Utf8; "+
			"DECLARE $chain AS Utf8; "+
			"UPSERT INTO $snapshotsext_gc (id, chain) VALUES ($id, $chain)",
		sql.Named("id", id),
		sql.Named("chain", chain))
	if err != nil {
		log.G(ctx).Error("addToGC: adding to GC failed", zap.Error(err))
		return kikimrError(err)
	}

	return nil
}

func (st *kikimrstorage) BeginDeleteSnapshotImpl(ctx context.Context, tx Querier, id string) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	// TODO: Lock on snapshot
	var state, base, tree, name, project string
	var public bool
	err := st.getSnapshotFields(ctx, tx, id, []string{"state", "base", "tree", "name", "project", "public"},
		[]interface{}{&state, nullString{&base}, &tree, &name, &project, &public})
	if err != nil {
		return err
	}

	if state == storage.StateDeleting || state == storage.StateDeleted || state == storage.StateRogueChunks {
		log.G(ctx).Info("BeginDeleteSnapshotImpl: double delete")
		return misc.ErrSnapshotNotFound
	}

	// Check that no children are being created
	var count int64
	err = tx.QueryRowContext(ctx,
		"#PREPARE "+
			"DECLARE $id AS Utf8; "+
			"DECLARE $state AS Utf8; "+
			"SELECT 1 FROM $snapshotsext_base as seb "+
			"INNER JOIN $snapshotsext as se on seb.id = se.id "+
			"WHERE seb.base = $id AND se.state = $state LIMIT 1",
		sql.Named("id", id),
		sql.Named("state", storage.StateCreating)).Scan(&count)
	switch err {
	case nil:
		log.G(ctx).Error("BeginDeleteSnapshotImpl: child creation in progress, locked", zap.Error(misc.ErrSnapshotLocked))
		return misc.ErrSnapshotLocked
	case sql.ErrNoRows:
	default:
		log.G(ctx).Error("BeginDeleteSnapshotImpl: child state query failed", zap.Error(err))
		return kikimrError(err)
	}

	// if we in GC and stoped at rogue chunks removal - continue it
	// if we failed even to set snapshot status to rogue chunk removal after write fail - do it still
	targetState := storage.StateDeleting
	if state == storage.StateRogueChunks || state == storage.StateCreating {
		targetState = storage.StateRogueChunks
	}

	t := time.Now()

	// Fix indices
	// Mark snapshot as deleted
	// TODO: This is after deletion mark to pass test on yql. Check
	_, err = tx.ExecContext(ctx,
		"#PREPARE "+
			"DECLARE $tree AS Utf8; "+
			"DECLARE $id AS Utf8; "+
			"DECLARE $base AS Utf8; "+
			"DECLARE $project AS Utf8; "+
			"DECLARE $name AS Utf8; "+
			"DECLARE $state AS Utf8; "+
			"DECLARE $timestamp AS Uint64; "+
			"$children = ("+
			"  SELECT id AS childID, $id AS id, $base as base "+
			"  FROM $snapshotsext_base WHERE base = $id"+
			"); "+
			"UPDATE $snapshotsext ON "+
			"  SELECT childID AS id, base FROM $children; "+
			"UPSERT INTO $snapshotsext_base (base, id) "+
			"  SELECT base, childID as id FROM $children "+
			"  WHERE base != ''; "+
			"DELETE FROM $snapshotsext_base ON "+
			"  SELECT id AS base, childID as id FROM $children; "+
			"DELETE FROM $snapshotsext_base ON (base, id) VALUES ($base, $id); "+
			"DELETE FROM $snapshotsext_tree ON (tree, id) VALUES ($tree, $id); "+
			"DELETE FROM $snapshotsext_statedescription ON (id) VALUES ($id); "+
			"DELETE FROM $snapshotsext_name ON (project, name) VALUES ($project, $name); "+
			"DELETE FROM $snapshotsext_public ON (id) VALUES ($id); "+
			"UPSERT INTO $snapshotsext (id, state, deleted, realsize, base) "+
			"  VALUES ($id, $state, $timestamp, 0, NULL); "+
			"UPSERT INTO $sizechanges (id, `timestamp`, realsize) "+
			"  VALUES ($id, $timestamp, 0)",
		sql.Named("tree", tree),
		sql.Named("id", id),
		sql.Named("base", base),
		sql.Named("project", project),
		sql.Named("name", name),
		sql.Named("state", targetState),
		sql.Named("timestamp", t))
	if err != nil {
		log.G(ctx).Error("BeginDeleteSnapshotImpl: update indices in front DB failed", zap.Error(err))
		return kikimrError(err)
	}

	// Add to garbage collection
	if err = st.addToGC(ctx, tx, id); err != nil {
		return err
	}

	return nil
}

func (st *kikimrstorage) endDeleteSnapshotImpl(ctx context.Context, tx Querier, id string) error {
	// Remove from chain tracking
	_, err := tx.ExecContext(ctx,
		"#PREPARE "+
			"DECLARE $id AS Utf8; "+
			"DECLARE $state AS Utf8; "+
			"DECLARE $chain AS Utf8; "+
			"UPDATE $snapshotsext ON (id, state) VALUES ($id, $state); "+
			"UPDATE $snapshotsext_gc ON (id, chain) VALUES ($id, $chain)",
		sql.Named("id", id), sql.Named("state", storage.StateDeleted), sql.Named("chain", id))
	if err != nil {
		log.G(ctx).Error("EndDeleteSnapshotImpl: update front DB/GC failed", zap.Error(err))
		return kikimrError(err)
	}
	return nil
}

func (st *kikimrstorage) DeleteTombstoneImpl(ctx context.Context, tx Querier, id string) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var state, project string
	err := st.getSnapshotFields(ctx, tx, id,
		[]string{"state", "project"}, []interface{}{&state, &project})
	if err != nil {
		return err
	}

	if state != storage.StateDeleted {
		log.G(ctx).Error("DeleteTombstoneImpl: invalid state",
			zap.String("expected", storage.StateDeleted), zap.String("actual", state))
		return misc.ErrNotATombstone
	}

	_, err = tx.ExecContext(ctx,
		"#PREPARE "+
			"DECLARE $project AS Utf8; "+
			"DECLARE $id AS Utf8; "+
			"DELETE FROM $snapshotsext_project ON (project, id) VALUES ($project, $id); "+
			"DELETE FROM $snapshotsext ON (id) VALUES ($id); "+
			"DELETE FROM $snapshotsext_gc ON (id) VALUES ($id); "+
			"DELETE FROM $sizechanges WHERE id = $id; "+
			"DELETE FROM $changed_children WHERE base = $id; ",
		sql.Named("project", project), sql.Named("id", id))
	if err != nil {
		log.G(ctx).Error("DeleteTombstoneImpl: delete failed", zap.Error(err))
		return kikimrError(err)
	}

	return nil
}

// clearContext encapsulates GC-related cleanup functionality.
type clearContext struct {
	st        *kikimrstorage
	id        string
	base      string
	tree      string
	chunksize int64
	size      int64
	deleted   time.Time
}

func (cc *clearContext) ClearSnapshotDataImpl(ctx context.Context, forceLock bool) (err error) {
	// Locks are used to prevent snapshot chunks map from concurrent modification.
	var lockFunc func(context.Context, string, string) (context.Context, storage.LockHolder, error)
	if forceLock {
		lockFunc = cc.st.LockSnapshotForce
	} else {
		lockFunc = cc.st.LockSnapshot
	}

	ctx, holder, err := lockFunc(ctx, cc.id, "ClearSnapshotDataImpl-delete-snapshot-data")
	if err != nil {
		return err
	}
	log.G(ctx).Debug("ClearSnapshotDataImpl: locks acquired")

	defer func() {
		holder.Close(ctx)
		log.G(ctx).Debug("ClearSnapshotDataImpl: locks released")
	}()

	err = cc.st.retryWithTx(ctx, "getDeltaFields", func(tx Querier) error {
		return cc.st.getDeltaFields(ctx, tx, cc.id, []string{"base", "tree"},
			[]interface{}{nullString{&cc.base}, &cc.tree})
	})
	switch err {
	case nil:
	case misc.ErrSnapshotNotFound:
		log.G(ctx).Info("ClearSnapshotDataImpl: snapshot disappeared during removal")
		return nil
	default:
		log.G(ctx).Error("ClearSnapshotDataImpl: getDeltaFields failre", zap.Error(err))
		return err
	}

	var state string
	err = cc.st.retryWithTx(ctx, "getSnapshotFields", func(tx Querier) error {
		return cc.st.getSnapshotFields(ctx, tx, cc.id, []string{"state", "deleted", "size", "chunksize"},
			[]interface{}{&state, kikimrTime{&cc.deleted}, &cc.size, &cc.chunksize})
	})
	switch err {
	case nil:
		// pass: no error
	case misc.ErrSnapshotNotFound:
		log.G(ctx).Warn("ClearSnapshotDataImpl: snapshot removed from front DB before chunk DB")
		return err
	default:
		log.G(ctx).Error("ClearSnapshotDataImpl: getSnapshotFields failre", zap.Error(err))
		return err
	}
	if cc.chunksize == 0 {
		err = cc.st.retryWithTx(ctx, "tryGetChunkSize", func(tx Querier) error {
			cc.chunksize, err = cc.st.tryGetChunkSize(ctx, tx, cc.tree, cc.id)
			return err
		})
		if err != nil {
			log.G(ctx).Error("ClearSnapshotDataImpl: tryGetChunkSize failre", zap.Error(err))
			return err
		}
	}

	if state != storage.StateDeleting && state != storage.StateFailed && state != storage.StateRogueChunks {
		log.G(ctx).Warn("ClearSnapshotDataImpl: unexpected state", zap.String("state", state))
		return xerrors.Errorf("clearSnapshotDataImpl: unexpected state=%v", state)
	}

	if state == storage.StateRogueChunks {
		if err = cc.deleteFailedChunks(ctx); err != nil {
			log.G(ctx).Error("failed to delete failed chunks", zap.Error(err))
			return err
		}
	}

	// We need to preserve sequence while deleting dependent snapshots
	// Hope that there are no other snapshots deleted the same microsecond
	var before int
	err = cc.st.retryWithTx(ctx, "checkChain", func(tx Querier) error {
		return tx.QueryRowContext(ctx,
			"#PREPARE "+
				"DECLARE $id AS Utf8; "+
				"DECLARE $deleted AS Uint64; "+
				"DECLARE $stateDeleted AS Utf8; "+
				"SELECT COUNT(*) FROM $snapshotsext_gc AS segc "+
				" INNER JOIN $snapshotsext_gc AS segc2 ON segc.chain = segc2.chain "+
				" INNER JOIN $snapshotsext AS s ON segc2.id = s.id "+
				" WHERE segc.id = $id AND segc.id != segc2.id AND s.deleted < $deleted AND s.state != $stateDeleted",
			sql.Named("id", cc.id),
			sql.Named("deleted", cc.deleted),
			sql.Named("stateDeleted", storage.StateDeleted)).Scan(&before)
	})
	if err != nil {
		log.G(ctx).Error("ClearSnapshotDataImpl: checkChain failre", zap.Error(err))
		return err
	}
	if before > 0 {
		log.G(ctx).Debug("ClearSnapshotDataImpl: cannot clear, waiting for previous", zap.Int("count", before))
		return misc.ErrAlreadyLocked
	}

	var snapMap map[int64]string
	err = cc.st.retryWithTx(ctx, "getOwnMap", func(tx Querier) error {
		snapMap, err = cc.getOwnMap(ctx, tx, cc.id, cc.chunksize, cc.size)
		return err
	})
	if err != nil {
		log.G(ctx).Error("ClearSnapshotDataImpl: getOwnMap failre", zap.Error(err))
		return err
	}
	if len(snapMap) == 0 {
		log.G(ctx).Warn("ClearSnapshotDataImpl: No own chunks found")
	}

	// New children can appear during deletion if non-locked child is deleted itself
	children := []string{"nothing"}
	for i := 0; i < childRebaseAttempts && len(children) > 0; i++ {
		// Get list of current children
		err = cc.st.retryWithTx(ctx, "getChildren", func(tx Querier) error {
			children, err = cc.getChildren(ctx, tx)
			return err
		})
		if err != nil {
			log.G(ctx).Error("ClearSnapshotDataImpl: getChildren failre", zap.Error(err))
			return err
		}

		// Rebase children on our base
		for _, childID := range children {
			err = cc.rebaseChild(ctx, childID, snapMap)
			switch err {
			case nil:
			case misc.ErrAlreadyLocked:
				continue
			default:
				log.G(ctx).Error("ClearSnapshotDataImpl: rebase failre", zap.Error(err))
				return err
			}
		}
	}
	if len(children) > 0 {
		log.G(ctx).Warn("ClearSnapshotDataImpl: failed to rebase children", zap.Int("count", len(children)))
		return misc.ErrAlreadyLocked
	}

	// Try batches of fixed size to avoid timeout
	queue := make([]storage.ChunkRef, 0, len(snapMap))

	for offset, chunkID := range snapMap {
		queue = append(queue, storage.ChunkRef{Offset: offset, ChunkID: chunkID})
	}
	sort.Slice(queue, func(i, j int) bool { return queue[i].Offset < queue[j].Offset })

	var lock sync.Mutex
	g, groupCtx := errgroup.WithContext(ctx)
	var spanStarter sync.Once
	for i := 0; i < cc.st.clearChunkWorkers; i++ {
		g.Go(func() error {
			var span opentracing.Span
			var removeCtx context.Context
			defer func() {
				if span != nil {
					tracing.Finish(span, err)
				}
			}()

			spanStarter.Do(func() {
				span, removeCtx = tracing.StartSpanFromContextFunc(groupCtx)
			})

			if span == nil {
				_, removeCtx = tracing.DisableSpans(groupCtx)
			}

			cfg, _ := config.GetConfig()
			tracingLimiter := tracing.SpanRateLimiter{
				InitialInterval: cfg.Tracing.LoopRateLimiterInitialInterval.Duration,
				MaxInterval:     cfg.Tracing.LoopRateLimiterMaxInterval.Duration,
			}

			var loopSpan opentracing.Span
			defer func() {
				if loopSpan != nil {
					tracing.Finish(loopSpan, err)
				}
			}()

			for {
				if loopSpan != nil {
					tracing.Finish(loopSpan, err)
				}
				var loopCtx context.Context
				loopSpan, loopCtx = tracingLimiter.StartSpanFromContext(removeCtx, "delete snapshot chunks")

				// Check that we do not have to finish yet
				select {
				case <-loopCtx.Done():
					return loopCtx.Err()
				default:
				}

				// Take our portion of work
				lock.Lock()
				if len(queue) == 0 {
					lock.Unlock()
					return nil
				}
				batchSize := cc.st.clearChunkBatchSize
				if batchSize > len(queue) {
					batchSize = len(queue)
				}
				batch := queue[:batchSize]
				queue = queue[batchSize:]
				lock.Unlock()

				err = cc.st.retryWithTx(loopCtx, "removeOwnChunks", func(tx Querier) error {
					return cc.removeOwnChunks(loopCtx, tx, batch)
				})
				if err != nil {
					return err
				}
			}

		})
	}
	if err = g.Wait(); err != nil {
		return err
	}

	err = cc.st.retryWithTx(ctx, "removeDelta", func(tx Querier) error {
		return cc.removeDelta(ctx, tx)
	})
	return err
}

func (cc *clearContext) getOwnMap(ctx context.Context, tx Querier, id string, chunkSize, size int64) (map[int64]string, error) {
	m := map[int64]string{}
	// NOTE: Same as in getChunks()
	// ScopedFunc for kikimrRows
	query := "#PREPARE " +
		"DECLARE $tree AS Utf8; " +
		"DECLARE $id AS Utf8; " +
		"DECLARE $start AS Int64; " +
		"DECLARE $end AS Int64; " +
		"SELECT chunkoffset, chunkid FROM $snapshotchunks " +
		" WHERE tree = $tree AND snapshotid = $id " +
		" AND chunkoffset >= $start AND chunkoffset < $end " +
		" ORDER BY chunkoffset ASC"
	args := []interface{}{sql.Named("tree", cc.tree), sql.Named("id", id), sql.Named("start", 0), sql.Named("end", 0)}
	var blockStartOffset, blockEndOffset, receivedOffset int64
	var actualCount int
	err := cc.st.selectPartial(ctx, tx, query, args, 0,
		func(rows SQLRows) error {
			var offset int64
			var chunkid string
			err := rows.Scan(&offset, &chunkid)
			if err != nil {
				log.G(ctx).Error("getOwnMap: scan failed", zap.Error(err))
				return kikimrError(err)
			}

			if offset%chunkSize != 0 {
				log.G(ctx).Error("getChunks: chunk with invalid offset")
				return xerrors.Errorf("getChunks: chunk with invalid offset=%v, chunkSize=%v", offset, chunkSize)
			}
			m[offset] = chunkid

			if offset > receivedOffset {
				receivedOffset = offset
			}
			actualCount++
			return nil
		},
		func() (q string, a []interface{}) {
			blockStartOffset, blockEndOffset = cc.st.calculateChunkRange(
				chunkSize, size, blockStartOffset, blockEndOffset, receivedOffset, actualCount)
			args[2] = sql.Named("start", blockStartOffset)
			args[3] = sql.Named("end", blockEndOffset)
			actualCount = 0

			if blockStartOffset >= size {
				return "", nil
			}

			return query, args
		})
	if err != nil {
		log.G(ctx).Error("getOwnMap: selectPartial failed", zap.Error(err))
	}

	return m, err
}

func (cc *clearContext) rebaseChild(ctx context.Context, childID string, snapMap map[int64]string) (err error) {
	ctx = log.WithLogger(ctx, log.GetLogger(ctx).With(logging.ChildID(childID)))

	ctx, holder, err := cc.st.LockSnapshot(ctx, childID, "rebaseChild-delete-snapshot-data")
	if err != nil {
		return err
	}

	defer func() {
		holder.Close(ctx)
		log.G(ctx).Debug("rebaseChild: locks released")
	}()

	log.G(ctx).Debug("rebaseChild: locks acquired")

	// Check that we are still a child
	var base string
	var oldRealSize int64
	err = cc.st.retryWithTx(ctx, "checkParent", func(tx Querier) error {
		return cc.st.getDeltaFields(ctx, tx, childID, []string{"base", "realsize"},
			[]interface{}{nullString{&base}, &oldRealSize})
	})
	switch err {
	case nil:
	case misc.ErrSnapshotNotFound:
		log.G(ctx).Info("child deleted before processing in parent")
		return nil
	default:
		return err
	}
	if base != cc.id {
		log.G(ctx).Warn("child rebased before processing in parent")
		return nil
	}

	var chunkSize, size int64
	err = cc.st.retryWithTx(ctx, "getChildInfo", func(tx Querier) error {
		return cc.st.getSnapshotFields(ctx, tx, childID, []string{"chunksize", "size"},
			[]interface{}{&chunkSize, &size})
	})
	switch err {
	case nil:
	case misc.ErrSnapshotNotFound:
		log.G(ctx).Info("child deleted from front DB before processing in parent")
		return nil
	default:
		return err
	}
	if chunkSize == 0 {
		err = cc.st.retryWithTx(ctx, "tryGetChunkSize", func(tx Querier) error {
			cc.chunksize, err = cc.st.tryGetChunkSize(ctx, tx, cc.tree, childID)
			return err
		})
		if err != nil {
			return err
		}
	}

	// Get child's chunk map
	var childMap map[int64]string
	err = cc.st.retryWithTx(ctx, "getOwnMap", func(tx Querier) error {
		childMap, err = cc.getOwnMap(ctx, tx, childID, chunkSize, size)
		return err
	})
	if err != nil {
		return err
	}

	// Calculate difference
	queue := make([]storage.ChunkRef, 0, len(snapMap))
	for offset, chunkID := range snapMap {
		if _, ok := childMap[offset]; ok {
			continue
		}
		queue = append(queue, storage.ChunkRef{Offset: offset, ChunkID: chunkID})
	}

	err = cc.st.CopyChunkRefs(ctx, childID, cc.tree, queue)
	if err != nil {
		return err
	}

	err = cc.st.retryWithTx(ctx, "finishRebase", func(tx Querier) error {
		// Fix broken links and realsize
		// NB: realsize is correct in case of interrupt due to calculation from childMap
		// But we must consider zero chunks which do not increase realsize.
		// Fix indices
		// Update realsize for living children
		// Update size changes for children alive at the time of parent deletion
		_, err = tx.ExecContext(ctx,
			"#PREPARE "+
				"DECLARE $id AS Utf8; "+
				"DECLARE $base AS Utf8; "+
				"DECLARE $tree AS Utf8; "+
				"DECLARE $childID AS Utf8; "+
				"DECLARE $old_real_size AS Int64; "+
				"DECLARE $chunk_size AS Int64; "+
				"DECLARE $timestamp AS Uint64; "+
				"$real_size = ("+
				"  SELECT COUNT_IF(NOT c.zero) * $chunk_size AS real_size "+
				"  FROM $snapshotchunks AS sc "+
				"  INNER JOIN $chunks AS c ON sc.chunkid = c.id "+
				"  WHERE sc.tree = $tree AND sc.snapshotid = $childID"+
				"); "+
				"$child = ("+
				"  SELECT state, deleted, $childID as childID, $base AS base "+
				"  FROM $snapshotsext WHERE id = $childID"+
				"); "+
				"UPDATE $snapshots ON (id, base, realsize) "+
				"  SELECT $childID AS id, $base AS base, real_size AS realsize "+
				"  FROM $real_size; "+
				"DELETE FROM $snapshots_base ON (base, id) VALUES ($id, $childID); "+
				"UPSERT INTO $snapshots_base (id, base) "+
				"  SELECT childID AS id, base "+
				"  FROM $child WHERE base != ''; "+
				"UPDATE $snapshotsext ON "+
				"  SELECT $childID AS id, rs.real_size AS realsize "+
				"  FROM $child AS c CROSS JOIN $real_size AS rs "+
				"  WHERE c.deleted IS NULL AND rs.real_size > $old_real_size; "+
				"UPSERT INTO $sizechanges (id, `timestamp`, realsize) "+
				"  SELECT $childID AS id, $timestamp AS `timestamp`, rs.real_size AS realsize "+
				"  FROM $child AS c CROSS JOIN $real_size AS rs "+
				"  WHERE (c.deleted IS NULL OR c.deleted > $timestamp) "+
				"    AND rs.real_size > $old_real_size; "+
				"UPSERT INTO $changed_children (base, id, `timestamp`, realsize) "+
				"  SELECT $id AS base, $childID AS id, $timestamp AS `timestamp`, rs.real_size AS realsize "+
				"  FROM $child AS c CROSS JOIN $real_size AS rs "+
				"  WHERE (c.deleted IS NULL OR c.deleted > $timestamp) "+
				"    AND rs.real_size > $old_real_size",
			sql.Named("id", cc.id),
			sql.Named("base", cc.base),
			sql.Named("tree", cc.tree),
			sql.Named("childID", childID),
			sql.Named("old_real_size", oldRealSize),
			sql.Named("chunk_size", cc.chunksize),
			sql.Named("timestamp", cc.deleted))
		if err != nil {
			log.G(ctx).Error("finishRebase: failed", zap.Error(err))
			return kikimrError(err)
		}
		return nil
	})
	return err
}

func (cc *clearContext) getChildren(ctx context.Context, tx Querier) ([]string, error) {
	var children []string
	query := "SELECT id FROM $snapshots_base WHERE base = $1 ORDER BY id ASC"
	args := []interface{}{cc.id}

	err := cc.st.selectPartial(ctx, tx, query, args, -1, func(rows SQLRows) error {
		var id string
		if err := rows.Scan(&id); err != nil {
			log.G(ctx).Error("getChildren: scan failed", zap.Error(err))
			return kikimrError(err)
		}
		children = append(children, id)
		return nil
	}, nil)
	if err != nil {
		log.G(ctx).Error("getChildren: selectPartial failed", zap.Error(err))
		return nil, err
	}
	return children, nil
}

func (cc *clearContext) removeDelta(ctx context.Context, tx Querier) (err error) {
	_, err = tx.ExecContext(ctx,
		"#PREPARE "+
			"DECLARE $tree AS Utf8; "+
			"DECLARE $id AS Utf8; "+
			"DECLARE $base AS Utf8; "+
			"DELETE FROM $snapshots_base ON (base, id) VALUES ($base, $id); "+
			"DELETE FROM $snapshots_tree ON (tree, id) VALUES ($tree, $id); "+
			"DELETE FROM $snapshots ON (id) VALUES ($id)",
		sql.Named("tree", cc.tree), sql.Named("id", cc.id), sql.Named("base", cc.base))
	if err != nil {
		log.G(ctx).Error("removeDelta: delete from chunk DB failed", zap.Error(err))
		return kikimrError(err)
	}
	return nil
}

func (cc *clearContext) removeOwnChunks(ctx context.Context, tx Querier, chunkBatch []storage.ChunkRef) (err error) {
	t := time.Now()
	defer misc.DeleteOwnChunksSpeed.ObserveContext(ctx, misc.SpeedSince(len(chunkBatch), t))

	declares := make([]string, 0, cc.st.clearChunkBatchSize)
	structs := make([]string, 0, cc.st.clearChunkBatchSize)
	for i := 0; i < cc.st.clearChunkBatchSize; i++ {
		declares = append(declares, fmt.Sprintf(
			"DECLARE $chunkoffset%d AS Int64; DECLARE $chunkid%d AS Utf8; ", i, i))
		structs = append(structs, fmt.Sprintf(
			"AsStruct($chunkoffset%d AS chunkoffset, $chunkid%d AS chunkid)", i, i))
	}

	// Argument count is constant to prepare only one query.
	args := make([]interface{}, 0, 2*cc.st.clearChunkBatchSize+2)
	args = append(args, sql.Named("tree", cc.tree), sql.Named("id", cc.id))
	argIndex := 0
	for _, chunk := range chunkBatch {
		args = append(args,
			sql.Named(fmt.Sprintf("chunkoffset%d", argIndex), chunk.Offset),
			sql.Named(fmt.Sprintf("chunkid%d", argIndex), chunk.ChunkID))
		argIndex++
	}
	// Fill empty slots with impossible values.
	for len(args) < cap(args) {
		args = append(args,
			sql.Named(fmt.Sprintf("chunkoffset%d", argIndex), int64(-1)),
			sql.Named(fmt.Sprintf("chunkid%d", argIndex), ""))
		argIndex++
	}

	// CLOUD-1825: mock for speedup
	if cc.st.fs != nil {
		// We have to select non-referenced chunks.
		query := "#PREPARE " +
			strings.Join(declares, "") +
			"$list = AsList(" + strings.Join(structs, ", ") + "); " +
			"SELECT c.id FROM AS_TABLE($list) as b " +
			"INNER JOIN $chunks AS c ON b.chunkid = c.id " +
			"WHERE c.refcnt == 1"

		var children []string
		err := cc.st.selectPartial(ctx, tx, query, args[2:], -1, func(rows SQLRows) error {
			var id string
			if err := rows.Scan(&id); err != nil {
				log.G(ctx).Error("getChildren: scan failed", zap.Error(err))
				return kikimrError(err)
			}
			children = append(children, id)
			return nil
		}, nil)
		if err != nil {
			return err
		}
		for _, chunkID := range children {
			_ = cc.st.fs.Delete(ctx, &storage.LibChunk{ID: chunkID})
		}
	}

	// Get unreferenced chunks
	// NB: We can't get refcnt in the beginning because refcnt may have changed
	// on parallel deletion of siblings
	// Remove old references
	// Decrement chunk refcnt
	// Remove unreferenced blobs
	// Remove unreferenced chunks
	query := "#PREPARE " +
		"DECLARE $tree AS Utf8; " +
		"DECLARE $id AS Utf8; " +
		strings.Join(declares, "") +
		"$list = AsList(" + strings.Join(structs, ", ") + "); " +
		"$refcounts = (SELECT c.id AS chunkid, c.refcnt AS refcnt " +
		"              FROM AS_TABLE($list) AS b " +
		"              INNER JOIN $chunks AS c ON b.chunkid = c.id); " +
		"DELETE FROM $snapshotchunks ON " +
		"  SELECT $tree AS tree, $id AS snapshotid, chunkoffset " +
		"  FROM AS_TABLE($list);" +
		"UPDATE $chunks ON " +
		"  SELECT chunkid AS id, refcnt-1 AS refcnt " +
		"  FROM $refcounts WHERE refcnt > 1; " +
		"DELETE FROM $chunks ON SELECT chunkid AS id " +
		"  FROM $refcounts WHERE refcnt <= 1; " +
		"DELETE FROM $blobs_on_hdd ON SELECT chunkid AS id " +
		"  FROM $refcounts WHERE refcnt <= 1;"

	_, err = tx.ExecContext(ctx, query, args...)
	if err != nil {
		log.G(ctx).Error("removeOwnChunks: failed", zap.Error(err))
		return kikimrError(err)
	}

	return nil
}

func (cc *clearContext) deleteFailedChunks(ctx context.Context) (err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	if cc.chunksize == 0 {
		log.G(ctx).Error("deleteFailedChunks should be called only afer clearContext initialization")
		return fmt.Errorf("chunk size is zero: snapshot_id=%s", cc.id)
	}

	allIDs := make([]string, 0, cc.size/cc.chunksize)
	for currentOffset := int64(0); currentOffset < cc.size; currentOffset += cc.chunksize {
		allIDs = append(allIDs, newChunkID(cc.id, currentOffset))
	}

	// This helps a little with sharding problems
	sort.Strings(allIDs)

	var lock sync.Mutex
	g, groupCtx := errgroup.WithContext(ctx)
	var spanStarter sync.Once
	for i := 0; i < cc.st.clearChunkWorkers; i++ {
		g.Go(func() error {
			var span opentracing.Span
			var removeCtx context.Context
			defer func() {
				if span != nil {
					tracing.Finish(span, err)
				}
			}()

			spanStarter.Do(func() {
				span, removeCtx = tracing.StartSpanFromContextFunc(groupCtx)
			})

			if span == nil {
				_, removeCtx = tracing.DisableSpans(groupCtx)
			}

			g := util.MakeLockGuard(&lock)
			defer g.UnlockIfLocked()

			cfg, _ := config.GetConfig()
			tracingLimiter := tracing.SpanRateLimiter{
				InitialInterval: cfg.Tracing.LoopRateLimiterInitialInterval.Duration,
				MaxInterval:     cfg.Tracing.LoopRateLimiterMaxInterval.Duration,
			}

			var loopSpan opentracing.Span
			defer func() {
				if loopSpan != nil {
					tracing.Finish(loopSpan, err)
				}
			}()

			for {
				if loopSpan != nil {
					tracing.Finish(loopSpan, err)
				}
				var loopCtx context.Context
				loopSpan, loopCtx = tracingLimiter.StartSpanFromContext(removeCtx, "delete rogue chunks bunch")

				// Check that we do not have to finish yet
				select {
				case <-loopCtx.Done():
					return loopCtx.Err()
				default:
				}

				// Take our portion of work
				g.Lock()
				if len(allIDs) == 0 {
					return nil
				}
				batchSize := cc.st.clearChunkBatchSize
				if batchSize > len(allIDs) {
					batchSize = len(allIDs)
				}
				batch := allIDs[:batchSize]
				allIDs = allIDs[batchSize:]
				g.Unlock()

				args := make([]interface{}, len(batch))
				listStatement := make([]string, len(batch))
				declarations := make([]string, len(batch))
				for i := range args {
					name := fmt.Sprintf("chunkid%d", i)
					args[i] = sql.Named(name, batch[i])
					declarations[i] = fmt.Sprintf("DECLARE $%s AS Utf8; ", name)
					listStatement[i] = fmt.Sprintf("AsStruct($%s AS chunkid)", name)
				}

				// DELETE FROM blobs_on_hdd WHERE id IN batch
				if err = cc.st.retryWithTx(loopCtx, "deleteFailedChunks", func(tx Querier) error {
					_, err := tx.ExecContext(loopCtx, "#PREPARE "+
						strings.Join(declarations, "")+
						"$list = AsList("+strings.Join(listStatement, ", ")+"); "+
						"DELETE FROM $blobs_on_hdd ON SELECT chunkid AS id FROM AS_TABLE($list)", args...)
					if err != nil {
						log.G(loopCtx).Error("deleteFailedChunks: Failed to delete batch", zap.Error(err))
						return kikimrError(err)
					}
					return nil
				}); err != nil {
					log.G(loopCtx).Error("deleteFailedChunks: failed to delete chunks", zap.Error(err))
					return err
				}

				// DELETE FROM chunks WHERE id IN batch
				if err = cc.st.retryWithTx(loopCtx, "deleteFailedChunks", func(tx Querier) error {
					_, err := tx.ExecContext(loopCtx, "#PREPARE "+
						strings.Join(declarations, "")+
						"$list = AsList("+strings.Join(listStatement, ", ")+"); "+
						"DELETE FROM $chunks ON SELECT chunkid AS id FROM AS_TABLE($list)", args...)
					if err != nil {
						log.G(loopCtx).Error("deleteFailedChunks: Failed to delete batch", zap.Error(err))
						return kikimrError(err)
					}
					return nil
				}); err != nil {
					log.G(loopCtx).Error("deleteFailedChunks: failed to delete chunks", zap.Error(err))
					return err
				}
			}
		})
	}
	err = g.Wait()

	if err == nil {
		err = cc.st.UpdateSnapshotStatus(ctx, cc.id, storage.StatusUpdate{
			State: storage.StateDeleting,
			Desc:  "snapshot deleting after removal of rogue chunks",
		})
	}

	return
}
