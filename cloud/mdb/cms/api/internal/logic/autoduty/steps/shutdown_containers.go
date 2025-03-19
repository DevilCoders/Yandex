package steps

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/x/math"
)

type ShutdownContainersStep struct {
	deploy deployapi.Client
	cfg    shipments.AwaitShipmentConfig
}

func (s *ShutdownContainersStep) GetStepName() string {
	return "shutdown containers"
}

func (s *ShutdownContainersStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	deployWrapper := s.shutdownDeployWrapper()
	meta := rd.D.OpsLog.StopContainers.Latest()
	if meta != nil && len(meta.GetShipments()) > 0 {
		return WaitShipment(ctx, meta, deployWrapper, rd)
	} else {
		opMeta := opmetas.NewStopContainersMeta()
		err := rd.D.OpsLog.Add(opMeta)
		if err != nil {
			return escalateWithErrAndMsg(err, "cannot set op meta")
		}
		return CreateShipment(ctx, rd.R.Fqnds, opMeta, deployWrapper)
	}
}

func (s *ShutdownContainersStep) shutdownDeployWrapper() shipments.AwaitShipment {
	awaitStopSec := math.MaxInt(int(s.cfg.Timeout)-1, 1) * 60
	stopRunningContainersCMD := fmt.Sprintf("if portoctl list -t | grep running ; then "+
		"portoctl list -t | grep running | cut -d \" \" -f 1 | xargs portoctl stop -T %d "+
		" ; fi", awaitStopSec)

	var cmds = []deployModels.CommandDef{{
		Type:    "cmd.run",
		Args:    []string{stopRunningContainersCMD},
		Timeout: encodingutil.FromDuration(time.Minute * time.Duration(s.cfg.Timeout)),
	}}
	return shipments.NewDeployWrapperFromCfg(
		cmds,
		s.deploy,
		s.cfg,
	)
}

func NewShutdownContainersStep(deploy deployapi.Client, cfg shipments.AwaitShipmentConfig) DecisionStep {
	return &ShutdownContainersStep{
		deploy: deploy,
		cfg:    cfg,
	}
}
