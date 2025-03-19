package alerting

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/mwswitch"
)

type ConditionType string

const ConditionByStepName ConditionType = "by-last-step-name"

type AlarmCondition struct {
	StepName        string        `json:"step_name" yaml:"step_name"`
	ConditionType   ConditionType `json:"type" yaml:"type"`
	CreatedMoreThan time.Duration `json:"created_more_than" yaml:"created_more_than"`
}

type AlertConfig struct {
	Conditions []AlarmCondition         `json:"alarm_condition" yaml:"alarm_condition"`
	EnabledMW  mwswitch.EnabledMWConfig `json:"enabled_mw" yaml:"enabled_mw"`
}

func DefaultAlertConfig() AlertConfig {
	return AlertConfig{Conditions: []AlarmCondition{
		{
			StepName:        steps.StepNameCheckIfPrimary,
			ConditionType:   ConditionByStepName,
			CreatedMoreThan: time.Hour * 24 * 10,
		},
	}}
}
