package steps_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
)

func TestCompareStatesAndReturn(t *testing.T) {
	type input struct {
		stateBefore *opmetas.Dom0StateMeta
		stateAfter  *opmetas.Dom0StateMeta
		onChanged   func() []steps.DecisionStep
		onUnchanged func() []steps.DecisionStep
	}
	type expectations struct {
		forHuman string
		steps    []steps.DecisionStep
		action   steps.AfterStepAction
	}
	type tCs struct {
		name   string
		in     input
		expect expectations
	}

	sw := "sw1"
	port1 := "p1"
	port2 := "p2"
	ip1 := "ip1"
	ip2 := "ip2"

	testCases := []tCs{
		{
			name: "nothing changed",
			in: input{
				stateBefore: &opmetas.Dom0StateMeta{
					SwitchPort: opmetas.SwitchPort{Switch: sw, Port: port1},
					IPs:        []string{ip1},
				},
				stateAfter: &opmetas.Dom0StateMeta{
					SwitchPort: opmetas.SwitchPort{Switch: sw, Port: port1},
					IPs:        []string{ip1},
				},
				onChanged: func() []steps.DecisionStep {
					return nil
				},
				onUnchanged: func() []steps.DecisionStep {
					return []steps.DecisionStep{}
				},
			},
			expect: expectations{
				forHuman: "Dom0 state was not changed",
				steps:    []steps.DecisionStep{},
				action:   steps.AfterStepContinue,
			},
		},
		{
			name: "switch or port was changed",
			in: input{
				stateBefore: &opmetas.Dom0StateMeta{
					SwitchPort: opmetas.SwitchPort{Switch: sw, Port: port1},
					IPs:        []string{ip1},
				},
				stateAfter: &opmetas.Dom0StateMeta{
					SwitchPort: opmetas.SwitchPort{Switch: sw, Port: port2},
					IPs:        []string{ip1},
				},
				onChanged: func() []steps.DecisionStep {
					return []steps.DecisionStep{}
				},
				onUnchanged: func() []steps.DecisionStep {
					return nil
				},
			},
			expect: expectations{
				forHuman: "yes: was switch \"sw1\" and port \"p1\", now switch \"sw1\" and port \"p2\"",
				steps:    []steps.DecisionStep{},
				action:   steps.AfterStepContinue,
			},
		},
		{
			name: "IPs were changed",
			in: input{
				stateBefore: &opmetas.Dom0StateMeta{
					SwitchPort: opmetas.SwitchPort{Switch: sw, Port: port1},
					IPs:        []string{ip1},
				},
				stateAfter: &opmetas.Dom0StateMeta{
					SwitchPort: opmetas.SwitchPort{Switch: sw, Port: port1},
					IPs:        []string{ip2},
				},
				onChanged: func() []steps.DecisionStep {
					return []steps.DecisionStep{}
				},
				onUnchanged: func() []steps.DecisionStep {
					return nil
				},
			},
			expect: expectations{
				forHuman: "yes: new ip2, missing ip1",
				steps:    []steps.DecisionStep{},
				action:   steps.AfterStepContinue,
			},
		},
	}

	for _, tc := range testCases {
		step := steps.NewDom0StateConditionStep(nil, tc.in.onChanged, tc.in.onUnchanged)
		t.Run(tc.name, func(t *testing.T) {
			result := step.CompareStatesAndReturn(tc.in.stateBefore, tc.in.stateAfter)
			require.NoError(t, result.Error)
			require.Equal(t, tc.expect.forHuman, result.ForHuman)
			require.Equal(t, tc.expect.steps, result.AfterMeSteps)
			require.Equal(t, tc.expect.action, result.Action)
		})
	}
}
