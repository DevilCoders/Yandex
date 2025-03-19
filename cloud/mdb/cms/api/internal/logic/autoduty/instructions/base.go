package instructions

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

type DecisionStrategy struct {
	EntryPoint steps.DecisionStep
}

type Instructions struct {
	Default  *DecisionStrategy
	Explicit map[string]*DecisionStrategy
}

func (i *Instructions) StrategyName(r models.ManagementRequest) string {
	return r.Name
}

func (i *Instructions) Find(r models.ManagementRequest) *DecisionStrategy {
	rName := i.StrategyName(r)
	if _, ok := i.Explicit[rName]; ok {
		return i.Explicit[rName]
	}
	return i.Default
}
