package storage

import (
	"context"

	"golang.org/x/xerrors"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

// Snapshot states
const (
	StateCreating    = "creating"
	StateReady       = "ready"
	StateDeleting    = "deleting"
	StateDeleted     = "deleted"
	StateFailed      = "failed"
	StateRogueChunks = "rogue-chunks"
)

// CheckReadableState checks whether snapshot can be read.
func CheckReadableState(ctx context.Context, state string) error {
	switch state {
	case StateCreating, StateFailed:
		log.G(ctx).Info("checkReadable failed: creating or failed", zap.String("state", state), zap.Error(misc.ErrSnapshotNotReady))
		return misc.ErrSnapshotNotReady
	case StateDeleting, StateDeleted, StateRogueChunks:
		log.G(ctx).Info("checkReadable failed: rogue chunks or deleting or deleted", zap.String("state", state), zap.Error(misc.ErrSnapshotNotFound))
		return misc.ErrSnapshotNotFound
	case StateReady:
	default:
		log.G(ctx).Warn("checkReadable: unknown state", zap.String("state", state))
		return xerrors.Errorf("checkReadable: unknown state, state=%v", state)
	}
	return nil
}

// CheckWriteableState checks whether snapshot can be written.
func CheckWriteableState(ctx context.Context, state string) error {
	switch state {
	case StateCreating:
	case StateDeleting, StateDeleted, StateRogueChunks:
		log.G(ctx).Info("checkWritable failed", zap.String("state", state), zap.Error(misc.ErrSnapshotNotFound))
		return misc.ErrSnapshotNotFound
	case StateReady:
		log.G(ctx).Error("checkWritable failed", zap.String("state", state), zap.Error(misc.ErrSnapshotReadOnly))
		return misc.ErrSnapshotReadOnly
	case StateFailed:
		err := xerrors.New("can't write into failed snapshot")
		log.G(ctx).Warn("checkWritable: failed state", zap.String("state", state), zap.Error(err))
		return err
	default:
		err := xerrors.Errorf("can't write into unknown state snapshot %s", state)
		log.G(ctx).Warn("checkWritable: unknown state", zap.String("state", state), zap.Error(err))
		return err
	}

	return nil
}
