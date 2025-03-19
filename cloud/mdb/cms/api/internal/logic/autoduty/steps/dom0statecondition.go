package steps

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/library/go/slices"
)

type Dom0StateConditionStep struct {
	deploy      deployapi.Client
	onChanged   func() []DecisionStep
	onUnchanged func() []DecisionStep
}

func (s *Dom0StateConditionStep) GetStepName() string {
	return "ip changed?"
}

func SlicesDifference(a, b []string) []string {
	var result []string
	for _, item := range a {
		if !slices.ContainsString(b, item) {
			result = append(result, item)
		}
	}
	return result
}

func (s *Dom0StateConditionStep) CompareStatesAndReturn(stateBefore, currentState *opmetas.Dom0StateMeta) RunResult {
	if stateBefore.SwitchPort.String() != currentState.SwitchPort.String() {
		return continueWithMessage(
			fmt.Sprintf("yes: was %s, now %s", stateBefore.SwitchPort.String(), currentState.SwitchPort.String()),
			s.onChanged()...)
	}

	newIPs := SlicesDifference(currentState.IPs, stateBefore.IPs)
	missingIPs := SlicesDifference(stateBefore.IPs, currentState.IPs)
	if len(newIPs) > 0 || len(missingIPs) > 0 {
		newIPsString := ""
		if len(newIPs) > 0 {
			newIPsString = fmt.Sprintf("new %s", strings.Join(newIPs, ", "))
		}
		oldIPsString := ""
		if len(missingIPs) > 0 {
			oldIPsString = fmt.Sprintf("missing %s", strings.Join(missingIPs, ", "))
		}
		return continueWithMessage(
			fmt.Sprintf("yes: %s", strings.Join([]string{newIPsString, oldIPsString}, ", ")),
			s.onChanged()...)
	}

	return continueWithMessage(
		"Dom0 state was not changed",
		s.onUnchanged()...)
}

func (s *Dom0StateConditionStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	// TODO: check non zero containers
	previousDom0state := rd.D.OpsLog.Dom0State
	if previousDom0state == nil || previousDom0state.SwitchPort.Switch == "" {
		return continueWithMessage("no switch/port were saved previously, this is not an error", s.onChanged()...)
	}
	afterWalleDom0State := rd.D.OpsLog.AfterWalleDom0State
	deployWrapper := dom0StateDeployWrapper(s.deploy)
	if afterWalleDom0State != nil && len(afterWalleDom0State.GetShipments()) > 0 {
		if afterWalleDom0State.SwitchPort.Switch != "" {
			return s.CompareStatesAndReturn(previousDom0state, afterWalleDom0State)
		}

		success, msg, err := switchPortFromShipment(ctx, s.deploy, afterWalleDom0State, rd)
		if success {
			return s.CompareStatesAndReturn(previousDom0state, afterWalleDom0State)
		} else if err != nil {
			return waitWithErrAndMessage(err, msg)
		} else {
			return waitWithMessage(msg)
		}
	}
	afterWalleDom0State = opmetas.NewDom0StateMeta()
	rd.D.OpsLog.AfterWalleDom0State = afterWalleDom0State
	return CreateShipment(ctx, []string{rd.R.MustOneFQDN()}, afterWalleDom0State, deployWrapper)
}

func NewDom0StateConditionStep(d deployapi.Client, onChanged, onUnchanged func() []DecisionStep) *Dom0StateConditionStep {
	return &Dom0StateConditionStep{
		deploy:      d,
		onChanged:   onChanged,
		onUnchanged: onUnchanged,
	}
}
