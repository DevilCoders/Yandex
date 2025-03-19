package move

import (
	"context"
	"sync"
	"time"

	"go.uber.org/zap"
	"golang.org/x/xerrors"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/go-common/util"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/chunker"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
)

type snapshotDevice struct {
	id        string
	size      int64
	blockSize int64
	st        storage.Storage
	// For src only
	chunks storage.ChunkMap
	// For dst only
	commit bool
	fail   bool

	checkAlg string

	holder    storage.LockHolder
	lockedCTX context.Context

	mtx           sync.Mutex
	metadata      []storage.LibChunk
	flushMetaFail error

	storeMetaBatchSize int
}

func (sd *snapshotDevice) VerifyMetadata(ctx context.Context) (err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	log.G(ctx).Info("starting metadata verification for snapshot", zap.String("snapshot_id", sd.id))
	// one worker because we not reading out all chunks
	report, err := chunker.NewVerifyContext(sd.st, 1, sd.id, false).VerifyChecksum(ctx)
	switch {
	case err != nil:
		return err
	case report.Status == chunker.StatusOk:
		return nil
	default:
		log.G(ctx).Error("snapshot metadata consistency is not verified", zap.Any("report", report), zap.String("snapshot_id", sd.id))
		return xerrors.Errorf("snapshot %s metadata is not verified %v: %w", sd.id, report, misc.ErrCorruptedSource)
	}
}

func (sd *snapshotDevice) GetBlockSize() int64 {
	return sd.blockSize
}

func (sd *snapshotDevice) GetSize() int64 {
	return sd.size
}

func (sd *snapshotDevice) ReadAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	ctx = misc.CompoundContext(ctx, sd.lockedCTX)

	chunk := sd.chunks.Get(offset)
	if chunk == nil {
		return false, false, nil
	}
	if chunk.Zero {
		return true, true, nil
	}
	t := time.Now()
	zero, err = sd.getChunker().ReadChunkBodyBuffer(ctx, sd.id, offset, data)
	if zero {
		misc.SnapshotReadZeroAtTimer.ObserveSince(t)
		misc.SnapshotReadAtSpeedZero.Observe(misc.SpeedSince(len(data), t))
	} else {
		misc.SnapshotReadAtTimer.ObserveSince(t)
		misc.SnapshotReadAtSpeed.ObserveContext(ctx, misc.SpeedSince(len(data), t))
	}
	return true, zero, err
}

func (sd *snapshotDevice) storeBatch(ctx context.Context, queue []storage.LibChunk) error {
	log.G(ctx).Info("Current metadata to store", zap.Int("queue_size", len(queue)))

	misc.CompoundContext(ctx, sd.lockedCTX)

	for len(queue) > 0 {
		batchSize := sd.storeMetaBatchSize
		if batchSize > len(queue) {
			batchSize = len(queue)
		}
		batch := queue[:batchSize]
		queue = queue[batchSize:]
		if err := sd.st.StoreChunksMetadata(ctx, sd.id, batch); err != nil {
			log.G(ctx).Error("Failed to store metadata batch", zap.Error(err), zap.Any("batch", batch))
			return err
		}
	}
	return nil
}

func (sd *snapshotDevice) Flush(ctx context.Context) (err error) {
	ctx = log.WithLogger(ctx, log.G(ctx).Named("Flush"))
	log.G(ctx).Debug("started")
	defer log.G(ctx).Debug("finished")

	// we should not cancel in the middle of flushing
	ctx = misc.WithNoCancel(ctx)

	guard := util.MakeLockGuard(&sd.mtx)
	defer guard.UnlockIfLocked()

	guard.Lock()
	if sd.flushMetaFail != nil {
		err = sd.flushMetaFail
		return
	}
	queue := make([]storage.LibChunk, len(sd.metadata))
	copy(queue, sd.metadata)
	sd.metadata = sd.metadata[:0]
	guard.Unlock()

	err = sd.storeBatch(ctx, queue)

	guard.Lock()
	if sd.flushMetaFail == nil {
		sd.flushMetaFail = err
	}
	guard.Unlock()
	return
}

func (sd *snapshotDevice) WriteAt(ctx context.Context, offset int64, data []byte) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	ctx = misc.CompoundContext(ctx, sd.lockedCTX)

	t := time.Now()
	c, err := sd.getChunker().StoreChunkBlob(ctx, sd.id, &chunker.StreamChunk{
		Offset: offset,
		Data:   data,
	})
	if err == nil {
		sd.mtx.Lock()
		sd.metadata = append(sd.metadata, c)
		err = sd.flushMetaFail
		sd.mtx.Unlock()
	}
	misc.SnapshotWriteAtTimer.ObserveSince(t)
	misc.SnapshotWriteAtSpeed.ObserveContext(ctx, misc.SpeedSince(len(data), t))
	return err
}

func (sd *snapshotDevice) WriteZeroesAt(ctx context.Context, offset int64, size int) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	t := time.Now()
	c, err := sd.getChunker().StoreZeroChunkBlob(ctx, sd.id, chunker.ZeroStreamChunk{
		Offset: offset,
		Size:   size,
	})
	if err == nil {
		sd.mtx.Lock()
		sd.metadata = append(sd.metadata, c)
		err = sd.flushMetaFail
		sd.mtx.Unlock()
	}
	misc.SnapshotWriteZeroAtTimer.ObserveSince(t)
	misc.SnapshotWriteAtSpeedZero.Observe(misc.SpeedSince(size, t))
	return err
}

func (sd *snapshotDevice) GetHint() directreader.Hint {
	return directreader.Hint{Size: sd.GetSize()}
}

func (sd *snapshotDevice) Close(ctx context.Context, err error) (res error) {
	defer func() {
		sd.holder.Close(ctx)
	}()

	log.G(ctx).Info("Closing snapshotDevice, storing metadata...", zap.Error(err))

	res = sd.Flush(ctx)
	log.DebugErrorCtx(ctx, res, "StoreChunksMetadata on Close snapshot device")
	if res != nil {
		return
	}

	if err == nil && sd.commit {
		log.G(ctx).Info("Closing snapshotDevice, calculating checksum...")
		if _, res = chunker.NewCalculateContext(sd.st, sd.id).UpdateChecksum(ctx); res != nil {
			log.G(ctx).Error("NewCalculateContext on commit snapshot failed", zap.Error(res))
			return
		}
		log.G(ctx).Info("Closing snapshotDevice, ending snapshot...")

		if res = sd.st.EndSnapshot(ctx, sd.id); res != nil {
			log.G(ctx).Error("EndSnapshot failed", zap.Error(res))
			return
		}
		log.G(ctx).Debug("snapshotDevice.Close: snapshot committed", logging.SnapshotID(sd.id))
	}

	if err != nil && sd.fail {
		log.G(ctx).Info("Closing snapshotDevice, changing snapshot status to RogueChunk...")
		if res = sd.st.UpdateSnapshotStatus(ctx, sd.id, storage.StatusUpdate{
			State: storage.StateRogueChunks,
			Desc:  err.Error(),
		}); res != nil {
			log.G(ctx).Error("UpdateSnapshotStatus(StateFailed) failed", zap.Error(res))
			return
		}

		// Some chunks can be still missing if we failed after chunk write and before metadata write
		log.DebugError(log.G(ctx), sd.st.ClearSnapshotData(ctx, sd.id), "ClearSnapshotData")

		// ClearData takes it's own locks
		sd.holder.Close(ctx)
		if res = sd.st.ClearSnapshotData(ctx, sd.id); res != nil {
			log.G(ctx).Error("ClearSnapshotData on failed snapshot failed", zap.Error(res))
			return
		}
	}
	return
}

func (sd *snapshotDevice) getChunker() *chunker.Chunker {
	return chunker.NewChunker(sd.st, chunker.MustBuildHasher(sd.checkAlg), chunker.NewLZ4Compressor)
}

func (mdf *DeviceFactory) newSnapshotMoveSrc(ctx context.Context, src *common.SnapshotMoveSrc, checkAlg string) (Src, error) {
	full, err := mdf.st.GetSnapshotFull(ctx, src.SnapshotID)
	if err != nil {
		return nil, err
	}

	if full.State.Code == storage.StateDeleting || full.State.Code == storage.StateDeleted {
		log.G(ctx).Error("newSnapshotMoveSrc: snapshot deleted", zap.String("state", full.State.Code))
		return nil, misc.ErrSnapshotNotFound
	}
	if full.State.Code != storage.StateReady {
		log.G(ctx).Error("newSnapshotMoveSrc: invalid state", zap.String("state", full.State.Code))
		return nil, misc.ErrSnapshotNotReady
	}

	c, holder, err := mdf.st.LockSnapshotShared(ctx, src.SnapshotID, "snapshotDevice-write-into-snapshot")
	if err != nil {
		return nil, err
	}

	sd := &snapshotDevice{
		id:        src.SnapshotID,
		st:        mdf.st,
		size:      full.Size,
		blockSize: full.ChunkSize,

		checkAlg: checkAlg,

		holder:    holder,
		lockedCTX: c,
	}

	chunks, err := sd.st.GetChunksFromCache(ctx, src.SnapshotID, false)
	if err != nil {
		return nil, err
	}

	sd.chunks = chunks
	return sd, nil
}

func (mdf *DeviceFactory) newSnapshotMoveDst(
	ctx context.Context,
	dst *common.SnapshotMoveDst,
	hint directreader.Hint,
	checkAlg string,
) (Dst, error) {
	full, err := mdf.st.GetSnapshotFull(ctx, dst.SnapshotID)
	switch err {
	case nil:
		if full.State.Code != storage.StateCreating {
			log.G(ctx).Error("newSnapshotMoveDst: invalid state", zap.String("state", full.State.Code))
			return nil, misc.ErrSnapshotReadOnly
		}

		if dst.Name != "" && full.Name != dst.Name {
			log.G(ctx).Error("newSnapshotMoveDst: snapshot with ID has different name", zap.String("full_name", full.Name),
				zap.String("dst_name", dst.Name))
			return nil, misc.ErrDuplicateID
		}

		c, holder, err := mdf.st.LockSnapshot(ctx, dst.SnapshotID, "snapshotDevice-write-into-snapshot")
		if err != nil {
			return nil, err
		}
		r := &snapshotDevice{
			id:        dst.SnapshotID,
			st:        mdf.st,
			size:      full.Size,
			blockSize: full.ChunkSize,
			commit:    dst.Commit,
			fail:      dst.Fail,

			checkAlg: checkAlg,

			holder:    holder,
			lockedCTX: c,

			storeMetaBatchSize: mdf.conf.Performance.StoreSnapshotMetadataBatchSize,
			metadata:           make([]storage.LibChunk, 0),
		}
		return r, nil
	case misc.ErrSnapshotNotFound:
		if !dst.Create {
			return nil, err
		}

		var ci common.CreationInfo
		ci.ID = dst.SnapshotID
		ci.ProjectID = dst.ProjectID
		ci.Name = dst.Name
		ci.ImageID = dst.ImageID
		size := misc.Ceil(hint.Size, storage.DefaultChunkSize)
		ci.Size = size
		if err := misc.FillIDIfAbsent(ctx, &ci.ID); err != nil {
			return nil, err
		}
		if len(dst.BaseSnapshotID) != 0 {
			if mdf.conf.General.ExperimentalIncrementalSnapshot {
				ci.Base = dst.BaseSnapshotID
			} else {
				log.G(ctx).Error("newSnapshotMoveDst: ExperimentalIncrementalSnapshot feature should be turned on", zap.String("dst_id", dst.SnapshotID))
				return nil, xerrors.Errorf("incrementalSnapshot should be turned on for snapshot_id=%s", dst.SnapshotID)
			}
		}

		_, err := mdf.st.BeginSnapshot(ctx, &ci)
		if err != nil {
			return nil, err
		}
		log.G(ctx).Debug("newSnapshotMoveDst: snapshot created", logging.SnapshotID(ci.ID))

		err = mdf.st.UpdateSnapshotStatus(ctx, ci.ID, storage.StatusUpdate{
			State: storage.StateCreating,
			Desc:  storage.BuildDescriptionFromProgress(0.0),
		})
		if err != nil {
			return nil, err
		}

		c, holder, err := mdf.st.LockSnapshot(ctx, dst.SnapshotID, "snapshotDevice-write-into-snapshot")
		if err != nil {
			return nil, err
		}
		r := &snapshotDevice{
			id:        ci.ID,
			st:        mdf.st,
			size:      size,
			blockSize: storage.DefaultChunkSize,
			commit:    dst.Commit,
			fail:      dst.Fail,

			checkAlg: checkAlg,

			holder:    holder,
			lockedCTX: c,

			storeMetaBatchSize: mdf.conf.Performance.StoreSnapshotMetadataBatchSize,
			metadata:           make([]storage.LibChunk, 0),
		}
		return r, nil
	default:
		return nil, err
	}
}
