package steps

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/library/go/core/log"
)

type LockRelease struct {
	locker lockcluster.Locker
}

func (s LockRelease) Name() string {
	return "lock release"
}

func (s LockRelease) RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, _ log.Logger) RunResult {
	state := stepCtx.State()
	err := s.locker.ReleaseCluster(ctx, state.Lock)
	if err != nil {
		return waitWithErrAndMessageFmt(err, "failed to release lock %q", state.Lock.LockID)
	}
	return continueWithMessageFmt("released lock %q", state.Lock.LockID)
}

func NewLockRelease(locker lockcluster.Locker) LockRelease {
	return LockRelease{
		locker: locker,
	}
}
