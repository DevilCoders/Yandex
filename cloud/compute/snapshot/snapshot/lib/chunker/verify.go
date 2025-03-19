package chunker

import (
	"sync"

	"go.uber.org/zap"
	"golang.org/x/net/context"
	"golang.org/x/sync/errgroup"
	"golang.org/x/xerrors"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
)

const (
	// StatusOk means the verification was successful.
	StatusOk = "ok"
	// StatusFailed means the verification has failed.
	StatusFailed = "failed"
)

type checksumContext struct {
	st storage.Storage
	id string

	sh     *storage.SnapshotFull
	chunks storage.ChunkMap
}

func (cc *checksumContext) setup(ctx context.Context) (err error) {
	cc.sh, err = cc.st.GetSnapshotFull(ctx, cc.id)
	if err != nil {
		log.G(ctx).Error("checksumContext. Can't get snapshot", zap.Error(err))
		return xerrors.Errorf("failed to get snapshot: %w", err)
	}

	// Drop from cache to avoid stale data
	cc.chunks, err = cc.st.GetChunksFromCache(ctx, cc.id, true)
	if err != nil {
		err = xerrors.Errorf("failed to get chunks: %w", err)
		log.G(ctx).Error("checksumContext. Can't get chunks from cache", zap.Error(err))
	}
	return err
}

// CalculateContext calculates and updates snapshot checksum.
type CalculateContext struct {
	checksumContext
}

// NewCalculateContext returns an CalculateContext instance.
func NewCalculateContext(st storage.Storage, id string) *CalculateContext {
	return &CalculateContext{
		checksumContext: checksumContext{
			st: st,
			id: id,
		},
	}
}

// UpdateChecksum calculates and updates snapshot checksum.
func (cc *CalculateContext) UpdateChecksum(ctx context.Context) (string, error) {
	t := misc.UpdateChecksumTimer.Start()
	defer t.ObserveDuration()
	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.Function("CalculateContext.UpdateChecksum")))

	err := cc.setup(ctx)
	if err != nil {
		log.G(ctx).Error("Can't setup calculate context.", zap.Error(err))
		return "", err
	}

	cfg, err := config.GetConfig()
	if err != nil {
		return "", err
	}

	checksum, err := calculateSnapshotChecksumFromChunks(cc.chunks, cc.sh.Size, cc.sh.ChunkSize, cfg.General.ChecksumAlgorithm)
	if err != nil {
		log.G(ctx).Error("Can't calculate snapshot checksum.", zap.Error(err))
		return "", err
	}
	err = cc.st.UpdateSnapshotChecksum(ctx, cc.id, checksum)
	if err != nil {
		log.G(ctx).Error("Can't update snapshot checksum.", zap.Error(err))
		return "", err
	}

	return checksum, nil
}

// VerifyContext validates snapshot and chunk checksums.
type VerifyContext struct {
	checksumContext

	workersCount int
	full         bool

	report     common.VerifyReport
	nextOffset int64
	mutex      sync.Mutex
}

// NewVerifyContext returns a VerifyContext instance.
func NewVerifyContext(st storage.Storage, workersCount int, id string, full bool) *VerifyContext {
	return &VerifyContext{
		checksumContext: checksumContext{
			st: st,
			id: id,
		},
		workersCount: workersCount,
		full:         full,
	}
}

// VerifyChecksum validates snapshot and chunk checksum.
func (vc *VerifyContext) VerifyChecksum(ctx context.Context) (*common.VerifyReport, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.Function("VerifyContext.VerifyChecksum")))
	t := misc.VerifyChecksumTimer.Start()
	defer t.ObserveDuration()

	ctx, holder, err := vc.st.LockSnapshotShared(ctx, vc.id, "VerifyContext-verify-snapshot-checksum")
	if err != nil {
		log.G(ctx).Error("Error while locking snapshot", zap.Error(err))
		return nil, err
	}
	defer holder.Close(ctx)

	err = vc.setup(ctx)
	if err != nil {
		log.G(ctx).Error("Can't setup calculate context.", zap.Error(err))
		return nil, err
	}

	err = vc.verifySnapshot(ctx)
	if err != nil {
		log.G(ctx).Error("Error while verify snapshot", zap.Error(err))
		return nil, err
	}

	vc.report.ID = vc.id
	vc.report.ChunkSize = vc.sh.ChunkSize
	vc.report.TotalChunks = vc.sh.Size / vc.sh.ChunkSize

	group, ctx := errgroup.WithContext(ctx)
	for i := 0; i < vc.workersCount; i++ {
		group.Go(func() error {
			return vc.verifyChunks(ctx)
		})
	}

	err = group.Wait()
	if err != nil {
		return nil, err
	}

	switch {
	case vc.report.Details == "" && len(vc.report.Mismatches) == 0:
		vc.report.Status = StatusOk
	case vc.report.Details != "" && len(vc.report.Mismatches) == 0:
		vc.report.Status = StatusFailed
		vc.report.Details = "missing chunks / corrupted snapshot checksum"
	case vc.report.Details == "" && len(vc.report.Mismatches) > 0:
		vc.report.Status = StatusFailed
		vc.report.Details = "corrupted chunk data"
	default:
		vc.report.Status = StatusFailed
		vc.report.Details = "corrupted chunk checksum / multiple errors"
	}

	return &vc.report, nil
}

func (vc *VerifyContext) verifyChunks(ctx context.Context) error {
	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		default:
		}

		vc.mutex.Lock()
		offset := vc.nextOffset
		vc.nextOffset += vc.sh.ChunkSize
		vc.mutex.Unlock()
		if offset >= vc.sh.Size {
			return nil
		}

		chunk := vc.chunks.Get(offset)
		chunkReport, err := vc.verifyChunk(ctx, chunk, offset)
		if err != nil {
			log.G(ctx).Error("verifyChunk failed", zap.Error(err), logging.ChunkID(chunk.ID), logging.SnapshotOffset(offset))
			return err
		}

		vc.mutex.Lock()
		vc.report.EmptyChunks += chunkReport.EmptyChunks
		vc.report.ZeroChunks += chunkReport.ZeroChunks
		vc.report.DataChunks += chunkReport.DataChunks
		vc.report.Mismatches = append(vc.report.Mismatches, chunkReport.Mismatches...)
		vc.mutex.Unlock()
	}
}

func (vc *VerifyContext) verifyChunk(ctx context.Context, chunk *storage.LibChunk, offset int64) (*common.VerifyReport, error) {
	report := &common.VerifyReport{
		Mismatches: make([]*common.ChunkMismatch, 0, 1),
	}

	switch {
	case chunk == nil:
		report.EmptyChunks++
	case chunk.Zero:
		report.ZeroChunks++
	default:
		report.DataChunks++
	}

	if chunk == nil || !vc.full {
		return report, nil
	}

	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.ChunkID(chunk.ID), logging.SnapshotOffset(offset)))

	if chunk.Zero {
		hc, err := buildHashConstructorFromSum(chunk.Sum)
		if err != nil {
			log.G(ctx).Error("VerifyChunk: build hash failed", zap.Error(err))
			report.Mismatches = append(report.Mismatches, &common.ChunkMismatch{
				Offset:  offset,
				ChunkID: chunk.ID,
				Details: "Unknown hash",
			})
			return report, nil
		}

		if chunk.Sum != zeroHash.Get(vc.sh.ChunkSize, hc()) {
			report.Mismatches = append(report.Mismatches, &common.ChunkMismatch{
				Offset:  offset,
				ChunkID: chunk.ID,
				Details: "checksum mismatch for zero chunk",
			})
		}
	} else {
		buf, err := vc.st.ReadBlob(ctx, chunk, false)
		if err != nil {
			log.G(ctx).Error("Can't ReadBlob.", zap.Error(err))
			return nil, err
		}
		if buf == nil {
			// No rows
			log.G(ctx).Error("VerifyChunk: data missing for non-zero chunk")
			report.Mismatches = append(report.Mismatches, &common.ChunkMismatch{
				Offset:  offset,
				ChunkID: chunk.ID,
				Details: "data missing for non-zero chunk",
			})
			return report, nil
		}

		_, hashsum, err := decodeChunkBody(chunk, buf)
		if err != nil {
			log.G(ctx).Error("VerifyChunk: decode failed", zap.Error(err))
			report.Mismatches = append(report.Mismatches, &common.ChunkMismatch{
				Offset:  offset,
				ChunkID: chunk.ID,
				Details: "decode failed: " + err.Error(),
			})
			return report, nil
		}

		if chunk.Sum != hashsum {
			report.Mismatches = append(report.Mismatches, &common.ChunkMismatch{
				Offset:  offset,
				ChunkID: chunk.ID,
				Details: "checksum mismatch",
			})
			return report, nil
		}
	}
	return report, nil
}

func (vc *VerifyContext) verifySnapshot(ctx context.Context) error {
	alg, err := getHashAlgFromSum(vc.sh.Checksum)
	if err != nil {
		return err
	}

	actualChecksum, err := calculateSnapshotChecksumFromChunks(vc.chunks, vc.sh.Size, vc.sh.ChunkSize, alg)
	if err != nil {
		log.G(ctx).Error("Can't calculate snapshot checksum", zap.Error(err))
		return err
	}

	if actualChecksum != vc.sh.Checksum {
		log.G(ctx).Warn("VerifySnapshot: snapshot checksum mismatch",
			zap.String("expected", vc.sh.Checksum),
			zap.String("actual", actualChecksum))
		vc.report.Details = "snapshot checksum mismatch"
	}
	vc.report.ChecksumMatched = actualChecksum == vc.sh.Checksum

	return nil
}
