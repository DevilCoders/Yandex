package instructions

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
)

const (
	atWalleFreq = 30 * time.Minute
)

func NewToReturnFromWalleStrategy(jglr juggler.API, dom0d dom0discovery.Dom0Discovery, d deployapi.Client) *DecisionStrategy {
	wf := NewCommonWorkflow("to return from walle", []steps.DecisionStep{
		steps.NewPeriodic(
			steps.ListOfSteps(
				steps.NewProlongDowntimesStep(jglr, dom0d),
				steps.NewUnregisterStep(d),
			),
			atWalleFreq,
		),
		steps.NewInParticularState(steps.AfterStepWait, "still at wall-e"),
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}
