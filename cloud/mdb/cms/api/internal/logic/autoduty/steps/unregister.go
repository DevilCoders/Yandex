package steps

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type UnregisterStep struct {
	deploy deployapi.Client
}

func (s *UnregisterStep) GetStepName() string {
	return "unregister minion"
}

func (s *UnregisterStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	action := rd.R.Name
	switch action {
	case models.ManagementRequestActionRedeploy, models.ManagementRequestActionPrepare:
	default:
		return continueWithMessage("not redeploy action, step skipped")
	}
	fqdn := rd.R.MustOneFQDN()
	_, err := s.deploy.GetMinionMaster(ctx, fqdn)
	if xerrors.Is(err, deployapi.ErrNotFound) {
		return continueWithMessage("not found in deploy")
	}
	if _, err := s.deploy.UnregisterMinion(ctx, fqdn); err != nil {
		if xerrors.Is(err, deployapi.ErrNotFound) {
			return continueWithMessage("not found in deploy")
		}
		return waitWithErrAndMessage(err, "deploy api failed")
	}
	return continueWithMessage("ok")
}

func NewUnregisterStep(deploy deployapi.Client) DecisionStep {
	return &UnregisterStep{
		deploy: deploy,
	}
}
