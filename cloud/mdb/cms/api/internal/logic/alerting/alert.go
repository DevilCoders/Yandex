package alerting

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/mwswitch"
	cms_models "a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type condHandler struct {
	matchesType  func(ctx context.Context, o cms_models.ManagementInstanceOperation, condition AlarmCondition) (bool, error)
	keepInResult func(o cms_models.ManagementInstanceOperation, condition AlarmCondition, now time.Time) bool
}

func DefaultKnownConditions(meta metadb.MetaDB, cfg mwswitch.EnabledMWConfig) map[ConditionType]condHandler {
	return map[ConditionType]condHandler{
		ConditionByStepName: {
			matchesType: func(ctx context.Context, o cms_models.ManagementInstanceOperation, condition AlarmCondition) (bool, error) {
				if len(o.ExecutedStepNames) > 0 && o.ExecutedStepNames[len(o.ExecutedStepNames)-1] == condition.StepName {
					host, err := meta.GetHostByVtypeID(ctx, o.InstanceID)
					if err != nil {
						return true, err
					}
					if mwswitch.IsAutomaticMasterWhipEnabledAlert(host.Roles, cfg) {
						return true, nil
					}
				}
				return false, nil
			},
			keepInResult: func(o cms_models.ManagementInstanceOperation, condition AlarmCondition, now time.Time) bool {
				return now.After(o.CreatedAt.Add(condition.CreatedMoreThan))
			},
		},
	}
}

func FilterOutAlertingOps(ctx context.Context, logger log.Logger, operations []cms_models.ManagementInstanceOperation, cfg AlertConfig, now time.Time, knownConditions map[ConditionType]condHandler) ([]cms_models.ManagementInstanceOperation, error) {
	var result []cms_models.ManagementInstanceOperation
	for _, o := range operations {
		var matchesAnyCondition = false
		for _, userDefinedCondition := range cfg.Conditions {
			if c, ok := knownConditions[userDefinedCondition.ConditionType]; !ok {
				ctxlog.Error(ctx, logger, "No handler for condition", log.String("condition", string(userDefinedCondition.ConditionType)))
			} else {
				match, err := c.matchesType(ctx, o, userDefinedCondition)
				if err != nil {
					return result, err
				}
				if match {
					matchesAnyCondition = true
					if c.keepInResult(o, userDefinedCondition, now) {
						result = append(result, o)
						break
					}
				}
			}
		}
		if !matchesAnyCondition {
			result = append(result, o)
		}
	}
	return result, nil
}
