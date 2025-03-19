package shipments

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type AwaitShipment struct {
	commands       []deployModels.CommandDef
	deploy         deployapi.Client
	parallelRuns   int64
	stopOnErrCount int64
	cmdTimeout     time.Duration
	skipTimeouts   bool
}

type AwaitShipmentConfig struct {
	Timeout         int64 `json:"timeout_minutes" yaml:"timeout_minutes"`
	StopOnErrCount  int64 `json:"max_errors" yaml:"max_errors"`
	MaxParallelRuns int64 `json:"parallel" yaml:"parallel"`
}

func (s *AwaitShipment) Wait(ctx context.Context, opMeta opmetas.DeployShipmentMeta) (WrapperWaitResult, error) {
	result := WrapperWaitResult{}
	shipments := opMeta.GetShipments()
	for fqdn, sIDs := range shipments {
		sID := sIDs[len(sIDs)-1]
		_, err := s.deploy.GetMinionMaster(ctx, fqdn)
		if err != nil {
			if xerrors.Is(err, deployapi.ErrNotFound) {
				result.Skipped.notFound = append(result.Skipped.notFound, fqdn)
				continue
			}
		}
		shipment, err := s.deploy.GetShipment(ctx, sID)
		if err != nil {
			return result, fmt.Errorf("failed to get shipment %s: %v", sID, err)
		}
		switch shipment.Status {
		case deployModels.ShipmentStatusInProgress:
			result.Progress = append(result.Progress, shipment)
		case deployModels.ShipmentStatusDone:
			result.Success = append(result.Success, shipment)
		case deployModels.ShipmentStatusTimeout:
			if s.skipTimeouts {
				result.Skipped.timeout = append(result.Skipped.timeout, shipment.FQDNs...)
			} else {
				result.Timeout = append(result.Timeout, shipment)
			}
		default:
			result.Failed = append(result.Failed, shipment)
		}
	}
	return result, nil
}

func (s *AwaitShipment) MessageForUserOnWait(ctx context.Context, workResult WrapperWaitResult, rd *types.RequestDecisionTuple) string {
	var success, progress, failed, timeout []string
	for _, item := range workResult.Success {
		success = append(success, item.FQDNs...)
	}
	for _, item := range workResult.Failed {
		ds, err := GetExtendedShipmentInfo(ctx, s.deploy, item)
		if err != nil {
			failed = append(failed, fmt.Sprintf("failed to get shipment info %s: %v", item.ID, err))
		}
		fs, err := FormatShipment(ds, item)
		if err != nil {
			failed = append(failed, fmt.Sprintf("failed to format shipment %s: %v", item.ID, err))
		}
		failed = append(failed, fs)
	}
	for _, item := range workResult.Timeout {
		timeout = append(timeout, item.FQDNs...)
	}
	for _, item := range workResult.Progress {
		progress = append(progress, item.FQDNs...)
	}
	response := fmt.Sprintf("%d success:%s",
		len(success), strings.Join(success, ","),
	)
	inProgessStr := ""
	if len(progress) > 0 {
		inProgessStr = fmt.Sprintf("\n%d in progress:%s", len(progress), strings.Join(progress, ","))
	}
	response += inProgessStr
	if len(failed) > 0 || len(timeout) > 0 {
		var button string
		switch rd.R.Status {
		case models.StatusInProcess:
			button = "OK -> AT-WALLE"
		case models.StatusOK:
			button = "MARK AS DONE"
		default:
			button = "please call velom@, this is a bug O_o"
		}
		response = fmt.Sprintf(
			`This is unrecoverable till MDB-9691 and/or MDB-9634.
Please check why errors happened. Then click %q button to continue.

%d failed:%s
%d timeout:%s
`,
			button,
			len(failed), strings.Join(failed, "\n"),
			len(timeout), strings.Join(timeout, ","),
		) + response
	}
	if len(workResult.Skipped.notFound) > 0 {
		response += fmt.Sprintf(`
%d ignored because not in current deploy: %s`, len(workResult.Skipped.notFound), strings.Join(workResult.Skipped.notFound, ","))
	}
	if len(workResult.Skipped.timeout) > 0 {
		response += fmt.Sprintf(`
%d ignored because skip timeouts: %s`, len(workResult.Skipped.timeout), strings.Join(workResult.Skipped.timeout, ","))
	}
	return response
}

func (s *AwaitShipment) CreateShipment(ctx context.Context, fqdns []string, opMeta opmetas.DeployShipmentMeta) WrapperCreateResult {
	result := WrapperCreateResult{}
	for _, fqdn := range fqdns {
		_, err := s.deploy.GetMinionMaster(ctx, fqdn)
		if err != nil {
			if xerrors.Is(err, deployapi.ErrNotFound) {
				result.AddNotInDeploy(fqdn)
				continue
			}
			result.Error = err
			break
		}
		shipment, err := s.deploy.CreateShipment(
			ctx,
			[]string{fqdn},
			s.commands,
			1,
			s.stopOnErrCount,
			s.cmdTimeout,
		)
		if err != nil {
			result.Error = err
			break
		}
		opMeta.SetShipment(fqdn, shipment.ID)
		result.AddSuccess(fqdn)
	}
	for _, fqdn := range fqdns {
		if slices.ContainsString(result.Success, fqdn) || slices.ContainsString(result.NotInDeploy, fqdn) {
			continue
		}
		result.AddNotCreated(fqdn)
	}
	return result
}

func NewDeployWrapperFromCfg(
	cmds []deployModels.CommandDef,
	deploy deployapi.Client,
	cfg AwaitShipmentConfig,
) AwaitShipment {
	return NewDeployWrapper(cmds, deploy, cfg.StopOnErrCount, cfg.MaxParallelRuns, cfg.Timeout)
}

func NewDeployWrapper(
	cmds []deployModels.CommandDef,
	deploy deployapi.Client,
	stopOnErr, parallel, timeoutMin int64,
	opts ...Option,
) AwaitShipment {
	s := AwaitShipment{
		commands:       cmds,
		deploy:         deploy,
		stopOnErrCount: stopOnErr,
		parallelRuns:   parallel,
		cmdTimeout:     time.Minute * (time.Duration(timeoutMin) + 5),
	}
	for _, opt := range opts {
		opt(&s)
	}
	return s
}

type Option func(shipment *AwaitShipment)

func WithSkipTimeouts() Option {
	return func(s *AwaitShipment) {
		s.skipTimeouts = true
	}
}
