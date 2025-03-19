package steps

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

type PreRestartStep struct {
	deploy deployapi.Client
	cfg    shipments.AwaitShipmentConfig
	dom0   dom0discovery.Dom0Discovery
}

func (s *PreRestartStep) GetStepName() string {
	return "pre_restart"
}

func (s *PreRestartStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	containers, err := containersFromDom0(ctx, s.dom0, rd.R.MustOneFQDN())
	if err != nil {
		return errorListingDBM(err)
	}
	if len(containers) == 0 {
		return continueWithMessage("no containers")
	}
	deployWrapper := s.shutdownDeployWrapper()
	meta := rd.D.OpsLog.PreRestart.Latest()
	if meta != nil && len(meta.GetShipments()) > 0 {
		return WaitShipment(ctx, meta, deployWrapper, rd)
	} else {
		opMeta := opmetas.NewPreRestartMeta()
		err := rd.D.OpsLog.Add(opMeta)
		if err != nil {
			return escalateWithErrAndMsg(err, "cannot set op meta")
		}
		return CreateShipment(ctx, containers, opMeta, deployWrapper)
	}
}

func (s *PreRestartStep) shutdownDeployWrapper() shipments.AwaitShipment {
	var cmds = []deployModels.CommandDef{{
		Type:    "cmd.run",
		Args:    []string{shipments.ShExecIfPresent("/usr/local/yandex/pre_restart.sh")},
		Timeout: encodingutil.FromDuration(time.Minute * time.Duration(s.cfg.Timeout)),
	}}
	return shipments.NewDeployWrapper(
		cmds,
		s.deploy,
		s.cfg.StopOnErrCount,
		s.cfg.MaxParallelRuns,
		10,
		shipments.WithSkipTimeouts(),
	)
}

func NewPreRestartStep(deploy deployapi.Client, dom0 dom0discovery.Dom0Discovery, cfg shipments.AwaitShipmentConfig) DecisionStep {
	return &PreRestartStep{
		deploy: deploy,
		cfg:    cfg,
		dom0:   dom0,
	}
}
