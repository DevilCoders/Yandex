package kikimr

import (
	"database/sql"
	"fmt"
	"strings"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

const (
	maxChunkSelectMutiplier = 10

	// Workaround YDB bug(?) restriction
	// CLOUD-25092
	copyChunkBatchSize = 49
)

func (st *kikimrstorage) ReadBlob(ctx context.Context, chunk *storage.LibChunk, mustExist bool) (data []byte, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	t := misc.KikimrReadChunkQueryTimer.Start()
	defer t.ObserveDuration()

	opts := sql.TxOptions{}
	opts.Isolation = sql.LevelReadCommitted
	opts.ReadOnly = true

	err = st.retryWithTxOpts(ctx, "ReadBlob", func(tx Querier) error {
		// TODO: Here we use RawBytes to avoid copy
		// We know that out driver (gokikimr) does not reuse the memory it returns.
		// This is a violation of database/sql contract that must be re-checked
		// on driver change.
		// NOTE: ReadCommitted + ROLLBACK are for performance.
		var raw sql.RawBytes
		rows, err := tx.QueryContext(ctx,
			"#PREPARE "+
				"DECLARE $id AS Utf8; "+
				"PRAGMA kikimr.IsolationLevel = 'ReadCommitted'; "+
				"SELECT data FROM $blobs_on_hdd WHERE id = $id; ROLLBACK", sql.Named("id", chunk.ID))
		if err != nil {
			log.G(ctx).Error("failed to load chunk", zap.Error(err))
			return kikimrError(err)
		}
		//nolint:errcheck
		defer rows.Close()

		if !rows.Next() {
			return sql.ErrNoRows
		}

		err = rows.Scan(&raw)
		if err != nil {
			return kikimrError(err)
		}
		data = []byte(raw)

		return misc.ErrNoCommit
	}, &opts)
	switch err {
	case nil:
		// CLOUD-1825: mock for speedup
		if st.fs != nil {
			data, err = st.fs.ReadChunkBody(ctx, chunk)
		}
		return
	case sql.ErrNoRows:
		if mustExist {
			log.G(ctx).Error("ReadBlob: data missing")
			return nil, misc.ErrNoSuchChunk
		}
		return nil, nil
	default:
		log.G(ctx).Error("ReadBlob: unknown error", zap.Error(err))
		return nil, kikimrError(err)
	}
}

func (st *kikimrstorage) GetChunksFromCache(ctx context.Context, snapshotID string, drop bool) (chunks storage.ChunkMap, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	var tree string
	var chunkSize, size int64
	if err = st.retryWithTxOpts(ctx, "GetChunksFromCache/GetSnapshotData", func(tx Querier) error {
		return st.getSnapshotFields(ctx, tx, snapshotID, []string{"size", "chunksize", "tree"},
			[]interface{}{&size, &chunkSize, &tree})
	}, &sql.TxOptions{
		Isolation: sql.LevelReadUncommitted,
		ReadOnly:  true,
	}); err != nil {
		return
	}

	if chunkSize == 0 {
		if err = st.retryWithTxOpts(ctx, "GetChunksFromCache/GetChunkSize", func(tx Querier) error {
			chunkSize, err = st.tryGetChunkSize(ctx, tx, tree, snapshotID)
			return err
		}, &sql.TxOptions{
			Isolation: sql.LevelReadUncommitted,
			ReadOnly:  true,
		}); err != nil {
			return
		}
	}

	if drop {
		st.cache.Remove(snapshotID)
	}

	err = st.retryWithTxOpts(ctx, "GetChunksFromCache", func(tx Querier) error {
		chunks, err = st.getChunksFromCache(ctx, tx, snapshotID, tree, chunkSize, size)
		return err
	}, &sql.TxOptions{
		Isolation: sql.LevelReadUncommitted,
		ReadOnly:  true,
	})
	if err != nil {
		log.G(ctx).Error("kikimrstorage.GetChunksFromCache can't get from db.", logging.SnapshotID(snapshotID),
			zap.Bool("drop", drop), zap.Error(err))
	}
	return
}

func (st *kikimrstorage) calculateChunkRange(chunkSize, size, blockStartOffset, blockEndOffset, receivedOffset int64, actualCount int) (int64, int64) {
	maxSelectRows := st.p.GetDatabase(defaultDatabase).GetConfig().MaxSelectRows

	var nextBlockStartOffset int64
	if actualCount < maxSelectRows {
		// First iteration goes here
		nextBlockStartOffset = blockEndOffset
	} else {
		nextBlockStartOffset = receivedOffset + chunkSize
	}
	if nextBlockStartOffset >= size {
		return size, size
	}

	// How many chunks we could receive from previous query
	maxCount := int((nextBlockStartOffset - blockStartOffset) / chunkSize)
	var newCount int
	switch {
	case nextBlockStartOffset == 0:
		// First iteration
		newCount = maxSelectRows
	case actualCount != 0 && maxCount/actualCount < maxChunkSelectMutiplier:
		newCount = maxSelectRows * maxCount / actualCount
	default:
		newCount = maxSelectRows * maxChunkSelectMutiplier
	}

	// Calculate new offset range [blockStartOffset, blockEndOffset) for selection.
	blockStartOffset = nextBlockStartOffset
	blockEndOffset = nextBlockStartOffset + int64(newCount)*chunkSize
	return blockStartOffset, blockEndOffset
}

func (st *kikimrstorage) getChunks(ctx context.Context, tx Querier, id, tree string, chunkSize, size int64) (storage.ChunkMap, error) {
	t := misc.GetChunksTimer.Start()
	defer t.ObserveDuration()

	result := storage.NewChunkMap()

	bases, err := st.getDeltaBaseList(ctx, tx, id, tree)
	if err != nil {
		return result, err
	}

	args := make([]interface{}, 0, len(bases)+2)
	args = append(args, sql.Named("a1", tree), 0, 0)

	var declareBuilder = &strings.Builder{}
	_, _ = declareBuilder.WriteString("DECLARE $a1 AS Utf8; ")
	_, _ = declareBuilder.WriteString("DECLARE $a2 AS Int64; ")
	_, _ = declareBuilder.WriteString("DECLARE $a3 AS Int64; ")

	var subq = &strings.Builder{}
	for i, b := range bases {
		if i > 0 {
			_, _ = subq.WriteString(" UNION ALL ")
		}
		argIndex := len(args) + 1
		_, _ = fmt.Fprintf(declareBuilder, "DECLARE $a%d AS Utf8; ", argIndex)
		_, _ = fmt.Fprintf(subq, "SELECT %d AS i, $a%d AS id", i, argIndex)
		args = append(args, sql.Named(fmt.Sprintf("a%d", argIndex), b))
	}

	// NOTE: It seems that having ChunkMap inmemory is not a really good solution
	// Not only to store it, but also to build we have to do lots of tricks

	// ScopedFunc for kikimrRows
	query := "#PREPARE " +
		declareBuilder.String() +
		"$subq = (" + subq.String() + "); " +
		"SELECT MIN_BY(c.id, sq.i), MIN_BY(c.sum, sq.i), MIN_BY(c.size, sq.i), " +
		"MIN_BY(c.zero, sq.i), MIN_BY(c.format, sq.i), " +
		"MIN_BY(sc.chunkoffset, sq.i) AS co " +
		"FROM $subq AS sq " +
		"INNER JOIN $snapshotchunks AS sc ON sq.id = sc.snapshotid " +
		"INNER JOIN $chunks AS c ON sc.chunkid = c.id " +
		"WHERE sc.tree = $a1 AND sc.chunkoffset >= $a2 AND sc.chunkoffset < $a3 " +
		"GROUP BY sc.chunkoffset ORDER BY co ASC"
	var blockStartOffset, blockEndOffset, receivedOffset int64
	var actualCount int
	err = st.selectPartial(ctx, tx, query, args, 0,
		func(rows SQLRows) error {
			chunk := &storage.LibChunk{}
			if err = rows.Scan(&chunk.ID, &chunk.Sum, &chunk.Size, &chunk.Zero,
				kikimrChunkFormat{&chunk.Format}, &chunk.Offset); err != nil {
				log.G(ctx).Error("getChunks: scan chunk failed", zap.Error(err))
				return kikimrError(err)
			}

			if chunk.Offset%chunkSize != 0 {
				log.G(ctx).Error("getChunks: chunk with invalid offset")
				return xerrors.Errorf("getChunks: chunk with invalid offset, offset=%v, chunkSize=%v", chunk.Offset, chunkSize)
			}

			result.Insert(chunk.Offset, chunk)

			if chunk.Offset > receivedOffset {
				receivedOffset = chunk.Offset
			}
			actualCount++
			return nil
		},
		func() (q string, a []interface{}) {
			blockStartOffset, blockEndOffset = st.calculateChunkRange(
				chunkSize, size, blockStartOffset, blockEndOffset, receivedOffset, actualCount)
			args[1] = sql.Named("a2", blockStartOffset)
			args[2] = sql.Named("a3", blockEndOffset)
			actualCount = 0

			if blockStartOffset >= size {
				return "", nil
			}

			return query, args
		})
	if err != nil {
		log.G(ctx).Error("getChunks: selectPartial failed", zap.Error(err), zap.String("query", query),
			zap.Any("args", args))
	}
	return result, err
}

func (st *kikimrstorage) CopyChunkRefs(ctx context.Context, id, tree string, queue []storage.ChunkRef) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	// Process in batches
	for len(queue) > 0 {
		scQB := newQueryBuilder()
		cQB := newQueryBuilder()

		for i := 0; i < len(queue) && i < copyChunkBatchSize; i++ {
			scQB.AppendInBraces(tree, id, queue[i].Offset, queue[i].ChunkID)
			cQB.AppendOne(queue[i].ChunkID)
		}
		queue = queue[len(cQB.clauses):]

		if len(scQB.clauses) == 0 {
			continue
		}

		// TODO(arovesto) once new driver enabled replace insecure sql edit with this
		// declare $ids as List<String>;
		// update $chunks set refcnt = refcnt + 1 where id in $ids
		rendered, err := cQB.RenderArgs()
		if err != nil {
			return err
		}

		err = st.retryWithTx(ctx, "CopyChunkRefs", func(tx Querier) error {
			// Copy references
			_, err := tx.ExecContext(ctx,
				"UPSERT INTO $snapshotchunks "+
					"(tree, snapshotid, chunkoffset, chunkid) "+
					"VALUES "+strings.Join(scQB.clauses, ","),
				scQB.args...)
			if err != nil {
				log.G(ctx).Error("CopyChunkRefs: reference copy failed", zap.Error(err))
				return kikimrError(err)
			}

			// Increment chunk refcnt
			_, err = tx.ExecContext(ctx,
				"UPDATE $chunks SET refcnt = refcnt + 1 WHERE id IN "+rendered)
			if err != nil {
				log.G(ctx).Error("CopyChunkRefs: refcnt increment failed", zap.Error(err))
				return kikimrError(err)
			}
			return nil
		})
		if err != nil {
			log.G(ctx).Error("kikimrstorage.CopyChunkRefs db error.", zap.Error(err),
				zap.Strings("scQB.clauses", scQB.clauses), zap.Any("scQB.args", scQB.args),
				zap.Strings("cQB.clauses", cQB.clauses), zap.Any("cQB.args", cQB.args))
			return err
		}
	}
	return nil
}

func (st *kikimrstorage) getChunksFromCache(ctx context.Context, tx Querier, id, tree string, chunkSize, size int64) (resMap storage.ChunkMap, resErr error) {
	if chunks, incache := st.cache.Get(id); incache {
		// NOTE: rely on the fact that chunks is a read-only thing
		return chunks, nil
	}

	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	chunks, err := st.getChunks(ctx, tx, id, tree, chunkSize, size)
	if err == nil {
		st.cache.Add(id, chunks)
	}

	return chunks, err
}

func (st *kikimrstorage) ReadChunkBody(ctx context.Context, snapshotID string, offset int64) (chunk *storage.LibChunk, data []byte, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.SnapshotID(snapshotID), logging.SnapshotOffset(offset)))

	var state, tree string
	var chunkSize, snapSize int64
	var chunks storage.ChunkMap

	err := st.retryWithTx(ctx, "ReadChunkBody", func(tx Querier) error {
		t := misc.KikimrGetSnapshotQueryTimer.Start()
		err := st.getSnapshotFields(ctx, tx, snapshotID, []string{"state", "size", "chunksize", "tree"},
			[]interface{}{&state, &snapSize, &chunkSize, &tree})
		if err != nil {
			return err
		}
		t.ObserveDuration()

		if err = storage.CheckReadableState(ctx, state); err != nil {
			return err
		}

		// Must not happen, db corruption.
		if chunkSize == 0 || snapSize == 0 {
			log.G(ctx).Error("ReadChunkBody: defaultChunkSize/snapSize is 0",
				zap.Int64("chunk_size", chunkSize), zap.Int64("snap_size", snapSize))
			return xerrors.Errorf("readChunkBody: defaultChunkSize/snapSize is 0, chunkSize=%v, snapSize=%v", chunkSize, snapSize)
		}

		// Do we ask for a valid chunk?
		if offset%chunkSize != 0 || offset+chunkSize > snapSize {
			log.G(ctx).Error("ReadChunkBody: no such chunk", zap.Error(misc.ErrNoSuchChunk),
				logging.SnapshotOffset(offset), zap.Int64("chunk_size", chunkSize),
				zap.Int64("snap_size", snapSize))
			return misc.ErrNoSuchChunk
		}

		chunks, err = st.getChunksFromCache(ctx, tx, snapshotID, tree, chunkSize, snapSize)
		return err
	})
	if err != nil {
		log.G(ctx).Error("Can't read chunk metadata from database", zap.Error(err))
		return nil, nil, err
	}

	chunk = chunks.Get(offset)
	if chunk == nil {
		return nil, make([]byte, chunkSize), nil
	}
	if chunk.Zero {
		return chunk, make([]byte, chunk.Size), nil
	}

	data, err = st.ReadBlob(ctx, chunk, true)
	if err != nil {
		log.G(ctx).Error("Can't read chunk blob", zap.Error(err), logging.ChunkID(chunk.ID))
	}
	return chunk, data, err
}

// Try to get size from chunk. Return default size if no chunks found.
func (st *kikimrstorage) tryGetChunkSize(ctx context.Context, tx Querier, snapshotID, tree string) (chunkSize int64, err error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.SnapshotID(snapshotID), zap.String("tree", tree)))

	// This function is used to calculate chunk size for non-committed snapshots.
	// It is rarely used, so we are OK with its cost.
	err = tx.QueryRowContext(ctx,
		"#PREPARE "+
			"DECLARE $tree AS Utf8; "+
			"DECLARE $id AS Utf8; "+

			"$any_of_chunks = ( "+
			"  SELECT chunkid "+
			"  FROM $snapshotchunks "+
			"  WHERE tree = $tree AND snapshotid = $id "+
			"  LIMIT 1 "+
			"); "+

			"SELECT c.size FROM $any_of_chunks AS sc "+
			"INNER JOIN $chunks AS c ON sc.chunkid = c.id; ",
		sql.Named("tree", tree), sql.Named("id", snapshotID)).Scan(&chunkSize)
	switch err {
	case nil:
		return
	case sql.ErrNoRows:
		log.G(ctx).Warn("Failed to get chunk size: no chunks found")
		return storage.DefaultChunkSize, nil
	default:
		log.G(ctx).Error("Failed to get chunk size", zap.Error(err))
		return 0, kikimrError(err)
	}
}
