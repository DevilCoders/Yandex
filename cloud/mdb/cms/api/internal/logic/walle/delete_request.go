package walle

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/statemachine"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

func (wi *WalleInteractor) DeleteRequest(ctx context.Context, user authentication.Result, taskID string) error {
	ctx, err := wi.cmsdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return err
	}
	defer wi.cmsdb.Rollback(ctx)

	if err := authorize(ctx, user); err != nil {
		return err
	}

	rd, err := wi.rdFromTaskID(ctx, taskID)
	if err != nil {
		return err
	}
	if err := wi.cmsdb.MarkRequestsCameBack(ctx, []models.AutomaticDecision{rd.D}); err != nil {
		return err
	}

	if err := wi.sm.TransitToState(
		ctx,
		rd,
		statemachine.Input{
			Action: steps.AfterStepClean,
		},
	); err != nil {
		return err
	}

	if err = wi.cmsdb.Commit(ctx); err != nil {
		return err
	}

	return nil
}

func (wi *WalleInteractor) rdFromTaskID(ctx context.Context, taskID string) (*types.RequestDecisionTuple, error) {
	reqs, err := wi.cmsdb.GetRequestsByTaskID(ctx, []string{taskID})
	if err != nil {
		return nil, err
	}
	r, ok := reqs[taskID]
	if !ok {
		return nil, semerr.NotFound("request not found")
	}
	decs, err := wi.cmsdb.GetDecisionsByRequestID(ctx, []int64{r.ID})
	if err != nil {
		return nil, err
	}
	if len(decs) == 0 {
		return nil, semerr.NotFound("decision not found")
	}
	d := decs[0]
	return &types.RequestDecisionTuple{
		R: r,
		D: d,
	}, nil
}
