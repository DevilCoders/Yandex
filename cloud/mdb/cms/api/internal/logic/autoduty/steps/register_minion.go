package steps

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type RegisterMinionStep struct {
	deploy deployapi.Client
	cfg    RegisterMinionStepConfig
}

type RegisterMinionStepConfig struct {
	GroupName string `json:"group" yaml:"group"`
}

func (s *RegisterMinionStep) GetStepName() string {
	return "register minion"
}
func (s *RegisterMinionStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	dom0FQDN := execCtx.GetActualRD().R.MustOneFQDN()
	_, err := s.deploy.GetMinion(ctx, dom0FQDN)
	if err == nil {
		return continueWithMessage("already registered")
	}
	if !xerrors.Is(err, deployapi.ErrNotFound) {
		return waitWithErrAndMessage(err, "failed to lookup minion, will try again")
	}
	_, err = s.deploy.CreateMinion(ctx, dom0FQDN, s.cfg.GroupName, true)
	if err != nil {
		return waitWithErrAndMessage(err, "failed to create minion, will try again")
	}
	return continueWithMessage("successfully created minion")
}

func NewRegisterMinionStep(
	deploy deployapi.Client,
	cfg RegisterMinionStepConfig,
) DecisionStep {
	return &RegisterMinionStep{
		deploy: deploy,
		cfg:    cfg,
	}
}
