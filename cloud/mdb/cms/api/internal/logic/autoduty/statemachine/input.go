package statemachine

import "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"

type Input struct {
	Action     steps.AfterStepAction
	MustReview bool
}
