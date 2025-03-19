package statemachine_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/statemachine"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func TestCartesianProduct(t *testing.T) {
	t.Run("adds missing", func(t *testing.T) {
		result := statemachine.CartesianProductForMissingCombinations(map[statemachine.SMRulesTC]statemachine.Transition{
			{models.DecisionNone, statemachine.Input{Action: steps.ProcessMe, MustReview: false}}: {Target: models.DecisionProcessing},
		})
		require.Equal(t, map[statemachine.SMRulesTC]statemachine.Transition{
			{models.DecisionNone, statemachine.Input{Action: steps.ProcessMe, MustReview: false}}: {Target: models.DecisionProcessing},
			{models.DecisionNone, statemachine.Input{Action: steps.ProcessMe, MustReview: true}}:  {Target: models.DecisionProcessing},
		}, result)
	})
	t.Run("does not touch existing", func(t *testing.T) {
		result := statemachine.CartesianProductForMissingCombinations(map[statemachine.SMRulesTC]statemachine.Transition{
			{models.DecisionNone, statemachine.Input{Action: steps.ProcessMe, MustReview: false}}: {Target: models.DecisionProcessing},
			{models.DecisionNone, statemachine.Input{Action: steps.ProcessMe, MustReview: true}}:  {Target: models.DecisionProcessing},
		})
		require.Equal(t, map[statemachine.SMRulesTC]statemachine.Transition{
			{models.DecisionNone, statemachine.Input{Action: steps.ProcessMe, MustReview: false}}: {Target: models.DecisionProcessing},
			{models.DecisionNone, statemachine.Input{Action: steps.ProcessMe, MustReview: true}}:  {Target: models.DecisionProcessing},
		}, result)
	})
}
