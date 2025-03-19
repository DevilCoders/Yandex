package steps

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Dom0StateStep struct {
	deploy deployapi.Client
}

func (s *Dom0StateStep) GetStepName() string {
	return "remember dom0 state"
}

func continueWithDom0State(state *opmetas.Dom0StateMeta) RunResult {
	return continueWithMessage(fmt.Sprintf("got %s, IPs: %v", state.SwitchPort.String(), state.IPs))
}

func (s *Dom0StateStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	// if switch/port is saved and known - do nothing
	// else
	//   if shipment not exists - create shipment
	//   else if shipment not ready - wait
	//        else read output from lldp and save switch/port
	rd := execCtx.GetActualRD()
	dom0state := rd.D.OpsLog.Dom0State
	deployWrapper := dom0StateDeployWrapper(s.deploy)
	if dom0state != nil && len(dom0state.GetShipments()) > 0 {
		if dom0state.SwitchPort.Switch != "" {
			return continueWithDom0State(dom0state)
		}

		success, msg, err := switchPortFromShipment(ctx, s.deploy, dom0state, rd)
		if success {
			return continueWithDom0State(dom0state)
		} else if err != nil {
			return waitWithErrAndMessage(err, msg)
		} else {
			return waitWithMessage(msg)
		}
	}
	dom0state = opmetas.NewDom0StateMeta()
	rd.D.OpsLog.Dom0State = dom0state
	return CreateShipment(ctx, []string{rd.R.MustOneFQDN()}, dom0state, deployWrapper)
}

func NewDom0StateStep(d deployapi.Client) *Dom0StateStep {
	return &Dom0StateStep{
		deploy: d,
	}
}

func dom0StateDeployWrapper(d deployapi.Client) shipments.AwaitShipment {
	const Timeout = 1
	var cmds = []deployModels.CommandDef{
		{
			Type:    "cmd.run",
			Args:    []string{"lldpctl -f json"},
			Timeout: encodingutil.FromDuration(time.Minute * time.Duration(Timeout)),
		},
		{
			Type:    "grains.get",
			Args:    []string{"fqdn_ip6"},
			Timeout: encodingutil.FromDuration(time.Minute * time.Duration(Timeout)),
		},
	}
	return shipments.NewDeployWrapper(
		cmds,
		d,
		1,
		1,
		3,
	)
}

func switchPortFromShipment(
	ctx context.Context,
	dapi deployapi.Client,
	state *opmetas.Dom0StateMeta,
	rd *types.RequestDecisionTuple,
) (bool, string, error) {
	wrapper := dom0StateDeployWrapper(dapi)
	workResult, err := wrapper.Wait(ctx, state)
	if err != nil {
		return false, "could not get info on shipment", err
	}
	if !workResult.IsSuccessful() {
		return false, wrapper.MessageForUserOnWait(ctx, workResult, rd), nil
	}
	if len(workResult.Success) != 1 {
		return false, fmt.Sprintf(
			"expected there will we exactly one successfull result, got %d, shipments %v",
			len(workResult.Success),
			state.GetShipments(),
		), nil
	}
	fs, err := shipments.GetExtendedShipmentInfo(ctx, dapi, workResult.Success[0])
	if err != nil {
		return false, "could not get info on shipment", err
	}
	outputs, err := shipments.GetOutputFromSimpleShipmentInfo(fs, rd.R.MustOneFQDN())
	if err != nil {
		return false, "could not get shipment output", err
	}

	if len(outputs) != 2 {
		return false, "", xerrors.Errorf("expected 2 cmd outputs, got %d", len(outputs))
	}

	swPort, err := parseLLDPOutput(outputs[0])
	if err != nil {
		return false, "could not parse LLDP output", err
	}

	ips, err := parseIPOutput(outputs[1])
	if err != nil {
		return false, "could not parse IPs output", err
	}

	state.SwitchPort = *swPort
	state.IPs = ips
	return true, "", nil
}

func parseLLDPOutput(output json.RawMessage) (*opmetas.SwitchPort, error) {
	var data string
	err := json.Unmarshal(output, &data)
	if err != nil {
		return nil, xerrors.Errorf("cast output of shipment to string %q: %w", string(output), err)
	}

	var lldp opmetas.LLDP
	err = json.Unmarshal([]byte(data), &lldp)
	if err != nil {
		return nil, xerrors.Errorf("cast output string to LLDP %q: %w", data, err)
	}
	swPorts := lldp.SwitchPorts()
	if len(swPorts) != 1 {
		return nil, xerrors.Errorf("expected only one Switch/Port, got %d: %v", len(swPorts), swPorts)
	}
	return &swPorts[0], nil
}

func parseIPOutput(output json.RawMessage) ([]string, error) {
	var ips []string
	err := json.Unmarshal(output, &ips)
	if err != nil {
		return nil, xerrors.Errorf("cast output of shipment to []string %q: %w", string(output), err)
	}
	return ips, nil
}
