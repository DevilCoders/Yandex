package duty

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
)

type CmsDom0DutyConfig struct {
	Juggler                  juggler.JugglerConfig `json:"juggler" yaml:"juggler"`
	PgIntTestCGroup          string                `json:"pgass_int_test_c_group" yaml:"pgass_int_test_c_group"`
	PgIntTestAllowAtOnce     int                   `json:"pgass_int_test_allow_at_once" yaml:"pgass_int_test_allow_at_once"`
	RespectADDecision        bool                  `json:"respect_autoduty_decision" yaml:"respect_autoduty_decision"`
	JugglerNamespaces        []string              `json:"downtime_juggler_namespaces" yaml:"downtime_juggler_namespaces"`
	OkHoursAtWalle           int                   `json:"ok_hours_at_walle" yaml:"ok_hours_at_walle"`
	OkFridayHoursAtWalle     int                   `json:"ok_friday_hours_at_walle" yaml:"ok_friday_hours_at_walle"`
	OkHoursUnfinished        int                   `json:"ok_hours_unfinished" yaml:"ok_hours_unfinished"`
	Steps                    steps.StepsCfg        `json:"steps" yaml:"steps"`
	DutyDecisionThresholdMin int                   `json:"duty_decision_threshold_min" yaml:"duty_decision_threshold_min"`
	MutatingDom0Limit        int                   `json:"mutating_dom0_limit" yaml:"mutating_dom0_limit"`
	GroupsWhiteL             []models.KnownGroups  `json:"groups_white_list" yaml:"groups_white_list"`
	GroupsBlackL             []string              `json:"groups_black_list" yaml:"groups_black_list"`
	UIHost                   string                `json:"ui_host" yaml:"ui_host"`
}

func DefaultConfig() CmsDom0DutyConfig {
	return CmsDom0DutyConfig{
		Juggler: juggler.DefaultJugglerConfig(),
	}
}
