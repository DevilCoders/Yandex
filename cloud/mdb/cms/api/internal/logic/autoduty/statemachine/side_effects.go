package statemachine

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func AnalysedByAutoDuty(ctx context.Context, cmsdb cmsdb.Client, rd *types.RequestDecisionTuple) error {
	if err := cmsdb.MarkRequestsAnalysedByAutoDuty(ctx, []models.AutomaticDecision{rd.D}); err != nil {
		return err
	}
	return nil
}

func SetAutodutyResolution(ctx context.Context, cmsdb cmsdb.Client, rd *types.RequestDecisionTuple) error {
	if err := cmsdb.SetAutoDutyResolution(ctx, []int64{rd.D.ID}, models.ResolutionApprove); err != nil {
		return err
	}
	return nil
}

func LetGoByAutoDuty(ctx context.Context, cmsdb cmsdb.Client, rd *types.RequestDecisionTuple) error {
	if err := cmsdb.MarkRequestsResolvedByAutoDuty(ctx, []models.AutomaticDecision{rd.D}); err != nil {
		return err
	}
	return nil
}

func FinishByAutoDuty(ctx context.Context, cmsdb cmsdb.Client, rd *types.RequestDecisionTuple) error {
	if err := cmsdb.MarkRequestsFinishedByAutoDuty(ctx, []models.AutomaticDecision{rd.D}); err != nil {
		return err
	}
	return nil
}
