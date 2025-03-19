package steps

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type HostForContainers struct {
	dbm      dbm.Client
	nonCntrs func() []DecisionStep
	forCntrs func() []DecisionStep
}

func (s *HostForContainers) GetStepName() string {
	return "dom0 registered in dbm"
}

func (s *HostForContainers) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	_, err := s.dbm.AreNewHostsAllowed(ctx, rd.R.MustOneFQDN())
	if err != nil {
		if xerrors.Is(err, dbm.ErrMissing) {
			return continueWithMessage("no", s.nonCntrs()...)
		}
		return escalateWithErrAndMsg(err, "could not get list of containers on host")
	}
	return continueWithMessage("yes", s.forCntrs()...)
}

func NewHostForContainers(
	dbm dbm.Client,
	nonCntrs func() []DecisionStep,
	forCntrs func() []DecisionStep) DecisionStep {
	return &HostForContainers{dbm: dbm, nonCntrs: nonCntrs, forCntrs: forCntrs}
}
