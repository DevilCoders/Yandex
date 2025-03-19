package steps

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/mwswitch"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthtypes "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type CheckIsMaster struct {
	isCompute      bool
	health         healthapi.MDBHealthClient
	finishWorkflow bool
	enabledCfg     mwswitch.EnabledMWConfig
}

func (s CheckIsMaster) Name() string {
	return StepNameCheckIfPrimary
}

func (s CheckIsMaster) finishStep(msg string) RunResult {
	if s.finishWorkflow {
		return finishWorkflowWithMessage(msg)
	} else {
		return continueWithMessage(msg)
	}
}

func (s CheckIsMaster) RunStep(ctx context.Context, opCtx *opcontext.OperationContext, l log.Logger) RunResult {
	state := opCtx.State().CheckIsMasterStep
	if state.Reason != "" {
		return continueWithMessage(state.Reason)
	}

	fqdn := opCtx.FQDN()
	ctxlog.Debug(ctx, l, "request health info about host neighbours", log.String("FQDN", fqdn))
	neighboursInfo, err := s.health.GetHostNeighboursInfo(ctx, []string{fqdn})
	if err != nil {
		return waitWithErrAndMessage(err, "failed request neighbours info")
	}

	ctxlog.Debug(ctx, l, "got response", log.Any("neighboursInfo", neighboursInfo))
	info, ok := neighboursInfo[fqdn]
	if !ok {
		// host can be unmanaged master db
		return continueWithMessage("health knows nothing about host")
	}

	if !info.IsHA() {
		state.Reason = "is not HA"
		state.IsNotMaster = true
		return s.finishStep(state.Reason)
	}

	health, err := s.health.GetHostsHealth(ctx, []string{fqdn})
	if err != nil {
		return waitWithErrAndMessage(err, "failed get host health")
	}

	for _, hh := range health {
		for _, service := range hh.Services() {
			if service.Timestamp().IsZero() {
				return waitWithMessage("unknown service status time")
			}
			if time.Since(service.Timestamp()) > 2*time.Minute {
				return waitWithMessageFmt("service %q status time is outdated: %q", service.Name(), service.Timestamp().String())
			}

			if service.Role() == healthtypes.ServiceRoleMaster {
				if s.isCompute {
					return waitWithMessageFmt(
						"Host is master for service %q and we are running in compute environment. "+
							mwswitch.IsAutomaticMasterWhipEnabledMessage(service.Name(), s.enabledCfg), service.Name(),
					)
				}
				state.Reason = fmt.Sprintf("host is master for service %q", service.Name())
				return continueWithMessageFmt(state.Reason)
			}
		}
	}

	state.Reason = "host is not master"
	state.IsNotMaster = true
	return s.finishStep(state.Reason)
}

func NewCheckIsMaster(isCompute bool, health healthapi.MDBHealthClient, finishWorkflow bool, cfg mwswitch.EnabledMWConfig) CheckIsMaster {
	return CheckIsMaster{
		isCompute:      isCompute,
		health:         health,
		finishWorkflow: finishWorkflow,
		enabledCfg:     cfg,
	}
}
