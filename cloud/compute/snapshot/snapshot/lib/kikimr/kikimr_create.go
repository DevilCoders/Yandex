package kikimr

import (
	"database/sql"
	"fmt"
	"math/rand"
	"strconv"
	"strings"
	"time"

	"go.uber.org/zap"
	"golang.org/x/net/context"
	"golang.org/x/xerrors"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

func (st *kikimrstorage) BeginSnapshot(ctx context.Context, ci *common.CreationInfo) (id string, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	t := misc.BeginSnapshotTimer.Start()
	defer t.ObserveDuration()

	log.G(ctx).Debug("Begin snapshot start", zap.Any("ci", ci))
	defer func() { log.G(ctx).Debug("Begin snapshot finish", zap.String("id", id), zap.Error(err)) }()

	if ci.Size <= 0 {
		log.G(ctx).Error("BeginSnapshot: invalid size")
		return "", misc.ErrInvalidSize
	}

	if ci.Name == "" {
		log.G(ctx).Error("BeginSnapshot: empty name")
		return "", misc.ErrInvalidName
	}

	err = st.retryWithTx(ctx, "BeginSnapshotImpl", func(tx Querier) error {
		id, err = st.BeginSnapshotImpl(ctx, tx, ci)
		return err
	})
	return
}

func (st *kikimrstorage) BeginChunk(ctx context.Context, id string, offset int64) error {
	return nil
}

func (st *kikimrstorage) StoreChunkData(ctx context.Context, snapshotID string, offset int64, data []byte) (err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	chunkID := newChunkID(snapshotID, offset)

	// CLOUD-1825: mock for speedup
	if st.fs != nil {
		if err = st.fs.StoreByID(ctx, chunkID, data); err != nil {
			return err
		}
		data = data[:0]
	}

	return st.retryWithTx(ctx, "StoreChunkData", func(tx Querier) error {
		_, err := tx.ExecContext(ctx, "#PREPARE "+
			"DECLARE $id AS Utf8; "+
			"DECLARE $data AS String; "+
			"UPSERT INTO $blobs_on_hdd (id, data) VALUES ($id, $data)", sql.Named("id", chunkID), sql.Named("data", data))
		if err != nil {
			log.G(ctx).Error("failed to upsert new chunk into db", zap.Error(err), zap.Int64("offset", offset))
			return kikimrError(err)
		}
		return nil
	})
}

// This function must be called inside LockSnapshot section
func (st *kikimrstorage) StoreChunksMetadata(ctx context.Context, snapshotID string, chunks []storage.LibChunk) (err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	var tree string
	if err = st.retryWithTx(ctx, "getDeltaFields", func(tx Querier) error {
		return st.getDeltaFields(ctx, tx, snapshotID, []string{"tree"},
			[]interface{}{&tree})
	}); err != nil {
		log.G(ctx).Error("failed to get snapshot tree", zap.Error(err))
		return err
	}

	declare := make([]string, 0, len(chunks)+2)
	chunksArgs := make([]string, 0, len(chunks))
	snapshotchunksArgs := make([]string, 0, len(chunks))
	args := make([]interface{}, 0, len(chunks)*6+2)

	declare = append(declare, "DECLARE $snapshotid AS Utf8; ", "DECLARE $tree AS Utf8; ")
	args = append(args, sql.Named("snapshotid", snapshotID), sql.Named("tree", tree))

	for i, c := range chunks {
		if !storage.IsValidFormat(c.Format) {
			log.G(ctx).Error("invalid chunk format found", zap.String("format", string(c.Format)), zap.Any("chunk", c))
			return misc.ErrInvalidFormat
		}
		c.ID = newChunkID(snapshotID, c.Offset)
		n := i + 1
		declare = append(declare, fmt.Sprintf("DECLARE $chunkid%d AS Utf8; "+
			"DECLARE $sum%d AS Utf8; "+
			"DECLARE $chunkoffset%d AS Int64; "+
			"DECLARE $format%d AS Utf8; "+
			"DECLARE $size%d AS Int64; "+
			"DECLARE $zero%d AS Bool; ", n, n, n, n, n, n))
		args = append(args, sql.Named(fmt.Sprintf("chunkid%d", n), c.ID),
			sql.Named(fmt.Sprintf("sum%d", n), c.Sum),
			sql.Named(fmt.Sprintf("chunkoffset%d", n), c.Offset),
			sql.Named(fmt.Sprintf("format%d", n), string(c.Format)), // replacer does not understand derived types
			sql.Named(fmt.Sprintf("size%d", n), c.Size),
			sql.Named(fmt.Sprintf("zero%d", n), c.Zero))
		chunksArgs = append(chunksArgs, fmt.Sprintf("($chunkid%d, $sum%d,$format%d,$size%d,$zero%d, 1)", n, n, n, n, n))
		snapshotchunksArgs = append(snapshotchunksArgs, fmt.Sprintf("($snapshotid, $tree, $chunkoffset%d, $chunkid%d)", n, n))
	}

	// no garbage created here, if snapshotchunks ok and chunks failed then deletion will go like so:
	// DELETE FROM chunks ON SELECT chunkid AS id FROM snapshotchunks WHERE snapshotid LIKE $snapshotID - ok, 0 chunks removed
	// DELETE FROM snapshotchunks WHERE snapshotid = id - ok
	if err := st.retryWithTx(ctx, "StoreChunksMetadata-snapshotchunks", func(tx Querier) error {
		_, err = tx.ExecContext(ctx, "#PREPARE "+strings.Join(declare, "")+" UPSERT INTO $snapshotchunks (snapshotid, tree, chunkoffset, chunkid) VALUES "+strings.Join(snapshotchunksArgs, ", "), args...)
		if err != nil {
			log.G(ctx).Error("failed to insert into snapshotchunks", zap.Error(err))
			return kikimrError(err)
		}
		return nil
	}); err != nil {
		return err
	}

	return st.retryWithTx(ctx, "StoreChunksMetadata-chunks", func(tx Querier) error {
		_, err := tx.ExecContext(ctx, "#PREPARE "+strings.Join(declare, "")+" UPSERT INTO $chunks (id, sum, format, size, zero, refcnt) VALUES "+strings.Join(chunksArgs, ", "), args...)
		if err != nil {
			log.G(ctx).Error("failed to insert into chunks", zap.Error(err))
			return kikimrError(err)
		}
		return nil
	})
}

func (st *kikimrstorage) EndChunk(ctx context.Context, snapshotID string, chunk *storage.LibChunk,
	offset int64, data []byte, progress float64) (err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	if progress < 0 || progress > 100 {
		log.G(ctx).Error("EndChunk failed", zap.Error(misc.ErrInvalidProgress))
		return misc.ErrInvalidProgress
	}

	// Initial generation for zero chunks
	chunk.ID = newChunkID(snapshotID, offset)

	t := misc.StoreChunkDB.Start()

	var p float64
	err = st.retryWithTx(ctx, "EndChunkImpl", func(tx Querier) error {
		p, err = st.EndChunkImpl(ctx, tx, snapshotID, chunk, offset, data)
		return err
	})
	if err != nil {
		return err
	}

	// Update progress sometimes
	if rand.Float64() < storage.UpdateProgressProbability {
		if progress == 0 {
			progress = p
		}
		err = st.retryWithTx(ctx, "UpdateSnapshotStatusImpl", func(tx Querier) error {
			return st.UpdateSnapshotStatusImpl(ctx, tx, snapshotID, storage.StatusUpdate{
				State: storage.StateCreating,
				Desc:  storage.BuildDescriptionFromProgress(float32(progress)),
			})
		})
		if err != nil {
			return err
		}
	}

	t.ObserveDuration()
	return nil
}

func (st *kikimrstorage) BeginSnapshotImpl(ctx context.Context, tx Querier, ci *common.CreationInfo) (resID string, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var q string

	var count int64
	err := tx.QueryRowContext(ctx,
		"#PREPARE "+
			"DECLARE $id AS Utf8; "+
			"SELECT 1 FROM $snapshotsext WHERE id = $id LIMIT 1",
		sql.Named("id", ci.ID)).Scan(&count)

	switch err {
	case nil:
		log.G(ctx).Error("BeginSnapshotImpl: snapshot ID already exist")
		return "", misc.ErrDuplicateID
	case sql.ErrNoRows:
		// ok
	default:
		log.G(ctx).Error("BeginSnapshotImpl: can't check uniqueness of a snapshot ID", zap.Error(err))
		return "", kikimrError(err)
	}

	tree := ci.ID

	err = tx.QueryRowContext(ctx,
		"#PREPARE "+
			"DECLARE $project AS Utf8; "+
			"DECLARE $name AS Utf8; "+
			"SELECT 1 FROM $snapshotsext_name WHERE project = $project AND name = $name LIMIT 1",
		sql.Named("project", ci.ProjectID), sql.Named("name", ci.Name)).Scan(&count)

	switch err {
	case nil:
		log.G(ctx).Error("BeginSnapshotImpl: snapshot name already exist")
		return "", misc.ErrDuplicateName
	case sql.ErrNoRows:
		// ok
	default:
		log.G(ctx).Error("BeginSnapshotImpl: can't check uniqueness of a snapshot name", zap.Error(err))
		return "", kikimrError(err)
	}

	if ci.Base != "" {
		var state string
		err := st.getSnapshotFields(ctx, tx, ci.Base, []string{"state", "tree"}, []interface{}{&state, &tree})
		if err != nil {
			return "", err
		}

		if err = storage.CheckReadableState(ctx, state); err != nil {
			return "", err
		}
	}

	// TODO: $snapshotsext_project is not cleared on snapshot deletion since it is used for billing
	timestamp := time.Now()
	q = "#PREPARE " +
		"DECLARE $id AS Utf8; " +
		"DECLARE $base AS Utf8; " +
		"DECLARE $tree AS Utf8; " +
		"DECLARE $state AS Utf8; " +
		"DECLARE $size AS Int64; " +
		"DECLARE $metadata AS Utf8; " +
		"DECLARE $organization AS Utf8; " +
		"DECLARE $project AS Utf8; " +
		"DECLARE $disk AS Utf8; " +
		"DECLARE $public AS Bool; " +
		"DECLARE $name AS Utf8; " +
		"DECLARE $description AS Utf8; " +
		"DECLARE $product_id AS Utf8; " +
		"DECLARE $image_id AS Utf8; " +
		"DECLARE $created AS Uint64; " +
		"$conditional = AsList(" +
		"  AsStruct($id AS id, $base AS base, $public AS public)" +
		"); " +
		"UPSERT INTO $snapshotsext " +
		"  (id, base, tree, state, size, metadata, organization, project, " +
		"  disk, public, name, description, productid, imageid, created, realsize, chunksize, " +
		"  checksum) " +
		"  VALUES ($id, $base, $tree, $state, $size, $metadata, $organization, $project, " +
		"  $disk, $public, $name, $description, $product_id, $image_id, $created, 0, 0, " +
		"  ''); " +
		"UPSERT INTO $snapshots (id, base, tree, chunksize) " +
		"  VALUES ($id, $base, $tree, 0); " +
		"UPSERT INTO $snapshotsext_base (id, base) " +
		"  SELECT id, base FROM AS_TABLE($conditional) " +
		"  WHERE base != ''; " +
		"UPSERT INTO $snapshots_base (id, base) " +
		"  SELECT id, base FROM AS_TABLE($conditional) " +
		"  WHERE base != ''; " +
		"UPSERT INTO $snapshotsext_tree (id, tree) VALUES ($id, $tree); " +
		"UPSERT INTO $snapshots_tree (id, tree) VALUES ($id, $tree); " +
		"UPSERT INTO $snapshotsext_project (id, project) VALUES ($id, $project); " +
		"UPSERT INTO $snapshotsext_name (project, name, id) VALUES ($project, $name, $id); " +
		"UPSERT INTO $snapshotsext_gc (id, chain) VALUES ($id, $id); " +
		"UPSERT INTO $snapshotsext_statedescription (id) VALUES ($id); " +
		"UPSERT INTO $snapshotsext_public (id) " +
		"  SELECT id FROM AS_TABLE($conditional) " +
		"  WHERE public == TRUE"
	_, err = tx.ExecContext(ctx, q,
		sql.Named("id", ci.ID),
		sql.Named("base", ci.Base),
		sql.Named("tree", tree),
		sql.Named("state", storage.StateCreating),
		sql.Named("size", ci.Size),
		sql.Named("metadata", ci.Metadata),
		sql.Named("organization", ci.Organization),
		sql.Named("project", ci.ProjectID),
		sql.Named("disk", ci.Disk),
		sql.Named("public", ci.Public),
		sql.Named("name", ci.Name),
		sql.Named("description", ci.Description),
		sql.Named("product_id", ci.ProductID),
		sql.Named("image_id", ci.ImageID),
		sql.Named("created", timestamp))
	if err != nil {
		log.G(ctx).Error("BeginSnapshotImpl: metadata DB failed", zap.Error(err))
		return "", kikimrError(err)
	}
	return ci.ID, nil
}

func (st *kikimrstorage) EndChunkImpl(ctx context.Context, tx Querier, id string, chunk *storage.LibChunk, offset int64, data []byte) (percent float64, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var (
		empty       float64
		state, tree string
		size        int64
	)
	err := st.getSnapshotFields(ctx, tx, id, []string{"state", "tree", "size"},
		[]interface{}{&state, &tree, &size})
	if err != nil {
		return empty, err
	}

	if err = storage.CheckWriteableState(ctx, state); err != nil {
		return empty, err
	}

	// CLOUD-1825: mock for speedup
	if st.fs != nil && !chunk.Zero {
		if err = st.fs.Store(ctx, chunk, data); err != nil {
			return empty, err
		}
		data = data[:0]
	}

	// Store info about chunk to attach to a snapshot
	// Store chunk related info for the blob
	// Store binary data
	_, err = tx.ExecContext(ctx,
		"#PREPARE "+
			"DECLARE $snapshot_id AS Utf8; "+
			"DECLARE $tree AS Utf8; "+
			"DECLARE $chunk_offset AS Int64; "+
			"DECLARE $chunk_id AS Utf8; "+
			"DECLARE $sum AS Utf8; "+
			"DECLARE $format AS Utf8; "+
			"DECLARE $size AS Int64; "+
			"DECLARE $zero AS Bool; "+
			"DECLARE $data AS String; "+
			"$conditional = AsList("+
			"  AsStruct($zero as zero)"+
			"); "+
			"UPSERT INTO $snapshotchunks (snapshotid, tree, chunkoffset, chunkid) "+
			"  VALUES ($snapshot_id, $tree, $chunk_offset, $chunk_id); "+
			"UPSERT INTO $chunks (id, sum, format, size, zero, refcnt) "+
			"  VALUES ($chunk_id, $sum, $format, $size, $zero, 1); "+
			"UPSERT INTO $blobs_on_hdd (id, data) "+
			"  SELECT $chunk_id AS id, $data AS data "+
			"  FROM AS_TABLE($conditional) WHERE zero = FALSE",
		sql.Named("snapshot_id", id),
		sql.Named("tree", tree),
		sql.Named("chunk_offset", offset),
		sql.Named("chunk_id", chunk.ID),
		sql.Named("sum", chunk.Sum),
		sql.Named("format", string(chunk.Format)),
		sql.Named("size", chunk.Size),
		sql.Named("zero", chunk.Zero),
		sql.Named("data", data))
	if err != nil {
		log.G(ctx).Error("EndChunkImpl: chunk db failed", zap.Error(err))
		return empty, kikimrError(err)
	}

	return float64(offset+chunk.Size) / float64(size) * 100, nil
}

// snapshot must be locker while EndSnapshot work
func (st *kikimrstorage) EndSnapshot(ctx context.Context, id string) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	t := misc.EndSnapshotTimer.Start()
	defer t.ObserveDuration()

	log.G(ctx).Debug("End snapshot start")
	defer func() {
		log.DebugErrorCtx(ctx, resErr, "End snapshot finish")
	}()

	// Select states. Base state may not exist, if snapshot has no base.
	var state, tree string
	var bstate *string
	var snapsize, bchunksize *int64
	var errTr error
	_ = st.retryWithTx(ctx, "end-snapshot-get-size", func(tx Querier) error {
		errTr = tx.QueryRowContext(ctx,
			"#PREPARE "+
				"DECLARE $id AS Utf8; "+
				"SELECT a.state, a.tree, a.size, b.state, b.chunksize FROM $snapshotsext AS a "+
				"LEFT JOIN $snapshotsext AS b "+
				"ON a.base = b.id WHERE a.id = $id",
			sql.Named("id", id)).Scan(&state, &tree, &snapsize, &bstate, &bchunksize)
		switch errTr {
		case nil:
			return nil
		case sql.ErrNoRows:
			log.G(ctx).Error("EndSnapshot: snapshot not found", zap.Error(misc.ErrSnapshotNotFound))
			return misc.ErrSnapshotNotFound
		default:
			log.G(ctx).Error("EndSnapshot: unknown error", zap.Error(errTr))
			return kikimrError(errTr)
		}
	})
	log.DebugErrorCtx(ctx, errTr, "EndSnapshot: get size", zap.String("tree", tree), zap.Stringp("bstate", bstate), zap.Int64p("snapsize", snapsize), zap.Int64p("bchunksize", bchunksize))
	if errTr != nil {
		return xerrors.Errorf("end snapshot get size: %w", errTr)
	}

	err := storage.CheckWriteableState(ctx, state)
	if err != nil {
		return err
	}

	// Check that base was not deleted during snapshot creation
	if bstate != nil {
		err = storage.CheckReadableState(ctx, *bstate)
		if err != nil {
			return err
		}
	}

	// Consistency check
	var sizes, occupied int64
	var size, nSize *int64
	offset := new(int64)
	*offset = -1

	q := "#PREPARE " +
		"DECLARE $tree AS Utf8; " +
		"DECLARE $id AS Utf8; " +
		"DECLARE $offset AS Int64; " +

		"$sn_chunks = ( " +
		"SELECT chunkid, chunkoffset " +
		"FROM $snapshotchunks " +
		"WHERE tree = $tree AND snapshotid = $id " +
		"AND chunkoffset > $offset " +
		"ORDER BY chunkoffset ASC " +
		"LIMIT " +
		strconv.Itoa(st.p.GetDatabase(defaultDatabase).GetConfig().MaxSelectRows) +
		"); " +

		"SELECT MAX(sc.chunkoffset), COUNT_IF(NOT c.zero), COUNT(DISTINCT c.size), SOME(c.size) " +
		"FROM $sn_chunks AS sc  " +
		"LEFT JOIN $chunks AS c ON sc.chunkid = c.id "
	args := []interface{}{sql.Named("tree", tree), sql.Named("id", id), sql.Named("offset", *offset)}
	var batch int64
	err = st.selectPartial(ctx, st.createTransactionPerQueryTx(&sql.TxOptions{Isolation: sql.LevelReadCommitted, ReadOnly: true}), "", nil, 0, func(rows SQLRows) error {
		err := rows.Scan(&offset, &batch, &sizes, &nSize)
		if err != nil {
			log.G(ctx).Error("EndSnapshotImpl: scan failed", zap.Error(err))
			return kikimrError(err)
		}

		if offset == nil || batch == 0 {
			q = ""
			return nil
		}

		occupied += batch
		args[2] = sql.Named("offset", *offset)

		if nSize == nil {
			if bchunksize != nil {
				nSize = bchunksize
			} else {
				s := int64(storage.DefaultChunkSize)
				nSize = &s
			}
		} else {
			if sizes > 1 {
				log.G(ctx).Error("EndSnapshotImpl: check consistency failed, many sizes", zap.Error(misc.ErrDifferentSize),
					zap.Int64("sizes", sizes))
				return misc.ErrDifferentSize
			}
			if bchunksize != nil && *bchunksize != *nSize {
				log.G(ctx).Error("EndSnapshotImpl: check consistency failed, bchunksize != nSize", zap.Error(misc.ErrDifferentSize),
					zap.Int64("bchunksize", *bchunksize), zap.Int64("nSize", *nSize))
				return misc.ErrDifferentSize
			}
			if size != nil && *size != *nSize {
				log.G(ctx).Error("EndSnapshotImpl: check consistency failed, size != nSize", zap.Error(misc.ErrDifferentSize),
					zap.Int64("size", *size), zap.Int64("nSize", *nSize))
				return misc.ErrDifferentSize
			}
		}

		size = new(int64)
		*size = *nSize

		return nil
	}, func() (string, []interface{}) {
		return q, args
	})

	if err == sql.ErrNoRows {
		// TODO: review: allow empty snapshots and deltas?
		// log.G(ctx).Error("EndSnapshotImpl: check consistency failed", zap.Error (misc.ErrSnapshotNotFull))
		// return misc.ErrSnapshotNotFull
	} else if err != nil {
		log.G(ctx).Error("EndSnapshotImpl: check consistency failed", zap.Error(err))
		return kikimrError(err)
	}

	if size == nil {
		if bchunksize != nil {
			size = bchunksize
		} else {
			s := int64(storage.DefaultChunkSize)
			size = &s
		}
	} else {
		if sizes > 1 || (bchunksize != nil && *bchunksize != *size) {
			log.G(ctx).Error("EndSnapshotImpl: check consistency failed", zap.Error(misc.ErrDifferentSize))
			return misc.ErrDifferentSize
		}
	}

	if (*snapsize)%(*size) != 0 {
		log.G(ctx).Error("EndSnapshotImpl: check consistency failed, size is not multiple of chunk size", zap.Error(misc.ErrInvalidSize))
		return misc.ErrInvalidSize
	}

	_ = st.retryWithTx(ctx, "commit snapshot", func(tx Querier) error {
		// use external err, don't redeclare it inside function
		_, err = tx.ExecContext(ctx,
			"#PREPARE "+
				"DECLARE $chunk_size AS Int64; "+
				"DECLARE $real_size AS Int64; "+
				"DECLARE $id AS Utf8; "+
				"DECLARE $timestamp AS Uint64; "+
				"DECLARE $state AS Utf8; "+
				"UPDATE $snapshots ON (id, chunksize, realsize) "+
				"  VALUES ($id, $chunk_size, $real_size); "+
				"UPSERT INTO $sizechanges (id, `timestamp`, realsize) "+
				"  VALUES ($id, $timestamp, $real_size); "+
				"UPDATE $snapshotsext ON (id, state, chunksize, realsize, statedescription) "+
				"  VALUES ($id, $state, $chunk_size, $real_size, ''); "+
				"DELETE FROM $snapshotsext_gc ON (id) VALUES ($id); "+
				"DELETE FROM $snapshotsext_statedescription ON (id) VALUES ($id)",
			sql.Named("chunk_size", *size),
			sql.Named("real_size", occupied*(*size)),
			sql.Named("id", id),
			sql.Named("timestamp", time.Now()),
			sql.Named("state", storage.StateReady))
		return err
	})
	if err != nil {
		log.G(ctx).Error("EndSnapshotImpl: failed", zap.Error(err))
		return kikimrError(err)
	}
	return nil
}
