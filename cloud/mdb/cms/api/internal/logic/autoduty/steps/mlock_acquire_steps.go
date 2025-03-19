package steps

import (
	"context"
	"fmt"
	"sort"
	"strconv"
	"strings"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type LockDom0Step struct {
	locker mlockclient.Locker
	holder string
}

func (s *LockDom0Step) GetStepName() string {
	return "lock dom0"
}

func LockIDForDom0(reqExtID string) string {
	return fmt.Sprintf("cms-dom0-%s", reqExtID)
}

func (s *LockDom0Step) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	dom0 := rd.R.MustOneFQDN()
	lockID := LockIDForDom0(rd.R.ExtID)
	return AcquireOrWaitDom0(
		ctx,
		s.locker,
		lockID,
		[]string{dom0},
		fmt.Sprintf("%q in %q", rd.R.Name, rd.R.ExtID),
		s.holder)
}

func AcquireOrWaitDom0(
	ctx context.Context,
	locker mlockclient.Locker,
	lockID string,
	objects []string,
	reason, holder string,
) RunResult {
	err := locker.CreateLock(ctx, lockID, holder, objects, reason)
	if err != nil {
		return waitWithErrAndMessage(err, fmt.Sprintf(
			"could not create lock %q",
			lockID,
		))
	}
	status, err := locker.GetLockStatus(ctx, lockID)
	if err != nil {
		return waitWithErrAndMessage(err, fmt.Sprintf(
			"could not get lock %q status",
			lockID,
		))
	}
	if !status.Acquired {
		conflictFQDNs := make([]string, len(status.Conflicts))
		for index, cnfl := range status.Conflicts {
			conflictFQDNs[index] = fmt.Sprintf(
				"%s locked by %s",
				cnfl.Object,
				strings.Join(cnfl.LockIDs, ", "),
			)
		}
		return waitWithMessage(fmt.Sprintf(
			"will try again. Could not acquire lock %q because of conflicts: %s",
			lockID,
			strings.Join(conflictFQDNs, ", "),
		))
	}
	return continueWithMessage(fmt.Sprintf(
		"acquired lock %q",
		lockID,
	))
}

func NewLockDom0Step(locker mlockclient.Locker, holder string) DecisionStep {
	return &LockDom0Step{
		locker: locker,
		holder: holder,
	}
}

type LockClusterNodesStep struct {
	locker lockcluster.Locker
	dscvr  dom0discovery.Dom0Discovery
}

func (s *LockClusterNodesStep) GetStepName() string {
	return "lock cluster nodes"
}

func (s *LockClusterNodesStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()

	if rd.D.OpsLog.LocksState != nil {
		locks := make([]string, 0, len(rd.D.OpsLog.LocksState.Locks))
		allAreTaken := true
		for _, lock := range rd.D.OpsLog.LocksState.Locks {
			locks = append(locks, lock.LockID)
			if !lock.IsTaken {
				allAreTaken = false
				break
			}
		}

		if allAreTaken {
			sort.Strings(locks)
			return continueWithMessage(fmt.Sprintf("acquired locks: %s", strings.Join(locks, ", ")))
		}
	}

	containers, err := s.dscvr.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return waitWithErrAndMessage(err, "could not get list of containers on dom0")
	}
	if len(containers.WellKnown) == 0 {
		return continueWithMessage("nothing to do")
	}

	state := opmetas.NewLocksStateMeta()
	locks := make([]string, 0)
	conflicts := make([]string, 0)
	for _, host := range containers.WellKnown {
		res, err := s.locker.LockCluster(ctx, host.FQDN, strconv.FormatInt(rd.D.ID, 10), lockcluster.WalleCMS)
		if err != nil {
			if xerrors.Is(err, lockcluster.NotAcquiredConflicts) {
				state.Locks[host.FQDN] = res
				reason, _ := lockcluster.ErrorReason(err)
				conflicts = append(conflicts, reason)
				continue
			}
			if xerrors.Is(err, lockcluster.UnmanagedHost) {
				continue
			}
			return waitWithErrAndMessage(err, "could not create lock for %s", host.FQDN)
		} else {
			state.Locks[host.FQDN] = res
			locks = append(locks, res.LockID)
		}
	}
	rd.D.OpsLog.LocksState = state

	if len(conflicts) > 0 {
		return waitWithMessage("will try again later.\nCreated locks: %s.\nConflicts:\n%s",
			strings.Join(locks, ", "),
			strings.Join(conflicts, "\n"),
		)
	}

	sort.Strings(locks)
	return continueWithMessage(fmt.Sprintf("acquired locks: %s", strings.Join(locks, ", ")))
}

func NewLockContainersStep(locker lockcluster.Locker, dscvr dom0discovery.Dom0Discovery) DecisionStep {
	return &LockClusterNodesStep{
		locker: locker,
		dscvr:  dscvr,
	}
}
