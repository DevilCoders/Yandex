package parallellimiter

import (
	"context"
	"strings"
	"time"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

const lockSuffix = "rateLimiter"

const OperationTypeTotal = "total"

type SharedLocker interface {
	LockSnapshotSharedMax(ctx context.Context, lockID string, reason string, maxActiveLocks int) (lockedCTX context.Context, holder storage.LockHolder, resErr error)
}

type OperationDescriptor struct {
	Type       string
	ProjectID  string
	CloudID    string
	ZoneID     string
	SnapshotID string
}

type RateLimiter struct {
	locker   SharedLocker
	settings RateLimiterSettings
	t        clockwork.Clock
}

type RateLimiterSettings struct {
	CheckInterval time.Duration

	MaxPerAZ       map[string]int
	MaxPerCloud    map[string]int
	MaxPerProject  map[string]int
	MaxPerSnapshot map[string]int
}

func NewLimiter(locker SharedLocker, settings RateLimiterSettings) *RateLimiter {
	return &RateLimiter{
		locker:   locker,
		settings: settings,
		t:        clockwork.NewRealClock(),
	}
}

// ctx must be active during operation
func (rl *RateLimiter) WaitQueue(ctx context.Context, d OperationDescriptor) (
	resCtx context.Context,
	resHolder storage.LockHolder,
	resErr error,
) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()
	ctxDone := ctx.Done()

	locksID := rl.generateLockIds(d)
	log.G(ctx).Debug("Generate lock ids", zap.Int("count", len(locksID)))

tryLock:
	for {
		if ctx.Err() != nil {
			return nil, nil, ctx.Err()
		}

		resCtx, resHolder, resErr = rl.getLocks(ctx, locksID...)
		if resErr == misc.ErrMaxShareLockExceeded {
			span.LogKV("message", "ErrMaxShareLockExceeded, wait and try again")
			log.G(ctx).Debug("ErrMaxShareLockExceeded, wait and try again")

			select {
			case <-ctxDone:
				return nil, nil, ctx.Err()
			case <-rl.t.After(rl.settings.CheckInterval):
				continue tryLock
			}
		}

		log.DebugErrorCtx(ctx, resErr, "Try get lock", zap.Any("op_description", d))

		return
	}
}

func (rl *RateLimiter) getLocks(ctx context.Context, lockDescriptors ...lockDescriptor) (resCtx context.Context, resHolder storage.LockHolder, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var holders orderedLocker
	defer func() {
		if resErr != nil {
			for i := len(holders) - 1; i >= 0; i-- {
				holders[i].Close(ctx)
			}
		}
	}()

	var err error
	var holder storage.LockHolder
	resCtx = ctx

	for _, ld := range lockDescriptors {
		if ld.Active {
			resCtx, holder, err = rl.locker.LockSnapshotSharedMax(resCtx, ld.LockID, ld.Name, ld.MaxCount)
			log.DebugErrorCtx(ctx, err, "Get lock", zap.String("lock_name", ld.Name), zap.String("lock_id", ld.LockID))
			if err != nil {
				return nil, nil, err
			}
			holders = append(holders, holder)
		} else {
			log.G(ctx).Debug("Skip lock", zap.String("lock_name", ld.Name))
		}
	}
	return resCtx, holders, nil
}

func (rl *RateLimiter) generateLockIds(od OperationDescriptor) []lockDescriptor {
	res := make([]lockDescriptor, 0, 8)
	for _, opType := range []string{od.Type, OperationTypeTotal} {
		res = append(res,
			makeLockDescriptor(rl.settings.MaxPerProject[opType], "project-type", od.ProjectID, od.Type),
			makeLockDescriptor(rl.settings.MaxPerCloud[opType], "cloud-type", od.CloudID, od.Type),
			makeLockDescriptor(rl.settings.MaxPerAZ[opType], "zone-type", od.ZoneID, od.Type),
			makeLockDescriptor(rl.settings.MaxPerSnapshot[opType], "snapshot-type", od.ProjectID, od.Type),
		)
	}

	return res
}

type lockDescriptor struct {
	Active   bool
	Name     string
	LockID   string
	MaxCount int
}

type orderedLocker []storage.LockHolder

func (ol orderedLocker) Close(ctx context.Context) {
	for i := len(ol) - 1; i >= 0; i-- {
		ol[i].Close(ctx)
	}
}

// makeLockDescriptor create lock
// it create inactive lock if maxCount == 0 (0 mean unlimited), ids is empty or if some of ids is empty string
func makeLockDescriptor(maxCount int, reason string, ids ...string) lockDescriptor {
	var res = lockDescriptor{
		Name:     reason,
		MaxCount: maxCount,
	}

	if maxCount == 0 || len(ids) == 0 {
		return res
	}

	for _, id := range ids {
		if id == "" {
			return res
		}
	}

	var idParts = make([]string, len(ids), len(ids)+2)
	copy(idParts, ids)
	idParts = append(idParts, reason, lockSuffix)

	res.LockID = strings.Join(idParts, "-")
	res.Active = true

	return res
}
