package steps

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
)

type UnlockDom0Step struct {
	locker mlockclient.Locker
}

func (s *UnlockDom0Step) GetStepName() string {
	return "unlock dom0"
}

func (s *UnlockDom0Step) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	lockID := LockIDForDom0(rd.R.ExtID)
	return UnlockOrWaitDom0(ctx, s.locker, lockID)
}

func NewUnlockDom0Step(locker mlockclient.Locker, holder string) DecisionStep {
	return &UnlockDom0Step{
		locker: locker,
	}
}

type UnlockClusterNodesStep struct {
	locker lockcluster.Locker
	dscvr  dom0discovery.Dom0Discovery
}

func (s *UnlockClusterNodesStep) GetStepName() string {
	return "unlock cluster nodes"
}

func (s *UnlockClusterNodesStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	if rd.D.OpsLog.LocksState == nil {
		return continueWithMessage("nothing to unlock")
	}
	containers, err := s.dscvr.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return waitWithErrAndMessage(err, "could not get list of containers on dom0")
	}
	if len(containers.WellKnown) == 0 {
		return continueWithMessage("nothing to do")
	}

	locks := make([]string, 0)
	for _, host := range containers.WellKnown {
		state, ok := rd.D.OpsLog.LocksState.Locks[host.FQDN]
		if !ok {
			continue
		}
		if err = s.locker.ReleaseCluster(ctx, state); err != nil {
			return waitWithErrAndMessage(err, "could not release lock for %q.\nReleased locks: %s",
				host.FQDN,
				strings.Join(locks, ", "),
			)
		}
		locks = append(locks, state.LockID)
	}
	return continueWithMessage(fmt.Sprintf("released locks: %s", strings.Join(locks, ", ")))
}

func NewUnlockClusterNodesStep(locker lockcluster.Locker, dscvr dom0discovery.Dom0Discovery) DecisionStep {
	return &UnlockClusterNodesStep{
		locker: locker,
		dscvr:  dscvr,
	}
}

func UnlockOrWaitDom0(ctx context.Context, locker mlockclient.Locker, lockID string) RunResult {
	err := locker.ReleaseLock(ctx, lockID)
	if err != nil {
		if semerr.IsNotFound(err) {
			return continueWithMessage(fmt.Sprintf("lock '%s' is not taken", lockID))
		}
		return waitWithErrAndMessage(err, fmt.Sprintf("failed to release lock '%s'", lockID))
	}
	return continueWithMessage(fmt.Sprintf("released lock '%s'", lockID))
}
