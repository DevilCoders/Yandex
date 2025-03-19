package steps

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type LockAcquire struct {
	locker lockcluster.Locker
}

func (s LockAcquire) Name() string {
	return "lock acquire"
}

func (s LockAcquire) RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, _ log.Logger) RunResult {
	state := stepCtx.State()

	if !state.Lock.IsTaken {
		fqdn := stepCtx.FQDN()
		res, err := s.locker.LockCluster(ctx, fqdn, stepCtx.OperationID(), lockcluster.InstanceCMS)
		if res != nil {
			state.SetLockState(res)
		}

		if err != nil {
			if xerrors.Is(err, lockcluster.NotAcquiredConflicts) {
				reason, _ := lockcluster.ErrorReason(err)
				return waitWithErrAndMessageFmt(err, "will try again: %s", reason)
			} else if xerrors.Is(err, lockcluster.UnmanagedHost) {
				return continueWithMessage("host is unmanaged")
			} else {
				if res != nil {
					return waitWithErrAndMessageFmt(err, "could not create lock %q", res.LockID)
				} else {
					return waitWithErrAndMessageFmt(err, "could not create lock for %s", fqdn)
				}
			}
		}
	}

	return continueWithMessageFmt(
		"acquired lock %q",
		state.Lock.LockID,
	)
}

func NewLockAcquire(locker lockcluster.Locker) LockAcquire {
	return LockAcquire{
		locker: locker,
	}
}
