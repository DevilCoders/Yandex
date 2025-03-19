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

type PostRestartStep struct {
	deploy deployapi.Client
	cfg    shipments.AwaitShipmentConfig
	dom0   dom0discovery.Dom0Discovery
}

func (s *PostRestartStep) GetStepName() string {
	return "post_restart containers"
}

func containersFromDom0(ctx context.Context, dom0D dom0discovery.Dom0Discovery, dom0fqdn string) ([]string, error) {
	containers, err := dom0D.Dom0Instances(ctx, dom0fqdn)
	if err != nil {
		return nil, err
	}
	fqdns := make([]string, len(containers.WellKnown))
	for ind, cnt := range containers.WellKnown {
		fqdns[ind] = cnt.FQDN
	}
	return fqdns, nil
}

func (s *PostRestartStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	containers, err := containersFromDom0(ctx, s.dom0, rd.R.MustOneFQDN())
	if err != nil {
		return errorListingDBM(err)
	}
	if len(containers) == 0 {
		return continueWithMessage("no containers")
	}
	deployWrapper := s.postRestartDeployWrapper()
	meta := rd.D.OpsLog.PostRestartContainers.Latest()
	if meta != nil && len(meta.GetShipments()) > 0 {
		return WaitShipment(ctx, meta, deployWrapper, rd)
	} else {
		opMeta := opmetas.NewPostRestartMeta()
		err := rd.D.OpsLog.Add(opMeta)
		if err != nil {
			return escalateWithErrAndMsg(err, "cannot set op meta")
		}
		return CreateShipment(ctx, containers, opMeta, deployWrapper)
	}
}

func (s *PostRestartStep) postRestartDeployWrapper() shipments.AwaitShipment {
	var cmds = []deployModels.CommandDef{{
		Type:    "cmd.run",
		Args:    []string{shipments.ShExecIfPresent("/usr/local/yandex/post_restart.sh")},
		Timeout: encodingutil.FromDuration(time.Minute * time.Duration(s.cfg.Timeout)),
	}}
	return shipments.NewDeployWrapper(
		cmds,
		s.deploy,
		s.cfg.StopOnErrCount,
		s.cfg.MaxParallelRuns,
		5*60,
	)
}

func NewPostRestartStep(deploy deployapi.Client, cfg shipments.AwaitShipmentConfig, dom0 dom0discovery.Dom0Discovery) DecisionStep {
	return &PostRestartStep{
		deploy: deploy,
		cfg:    cfg,
		dom0:   dom0,
	}
}
