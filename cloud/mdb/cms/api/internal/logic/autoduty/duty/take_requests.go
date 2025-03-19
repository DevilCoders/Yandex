package duty

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func (ad *AutoDuty) rdsToAnalyse(ctx context.Context, execCtx *steps.InstructionCtx) (*types.RequestDecisionTuple, error) {
	decisions, err := ad.cmsdb.GetDecisionsToProcess(ctx, doneDecisionsFromCtx(execCtx))
	if err != nil {
		return nil, err
	}
	return ad.rdFromDecision(ctx, decisions)
}

func (ad *AutoDuty) rdsToLetTo(ctx context.Context, execCtx *steps.InstructionCtx) (*types.RequestDecisionTuple, error) {
	decisions, err := ad.cmsdb.GetDecisionsToLetGo(ctx, doneDecisionsFromCtx(execCtx))
	if err != nil {
		return nil, err
	}
	return ad.rdFromDecision(ctx, decisions)
}

func (ad *AutoDuty) rdsToReturnFromWalle(ctx context.Context, execCtx *steps.InstructionCtx) (*types.RequestDecisionTuple, error) {
	decisions, err := ad.cmsdb.GetDecisionsToReturnFromWalle(ctx, doneDecisionsFromCtx(execCtx))
	if err != nil {
		return nil, err
	}
	return ad.rdFromDecision(ctx, decisions)
}

func (ad *AutoDuty) rdsToFinish(ctx context.Context, execCtx *steps.InstructionCtx) (*types.RequestDecisionTuple, error) {
	decisions, err := ad.cmsdb.GetDecisionsToFinishAfterWalle(ctx, doneDecisionsFromCtx(execCtx))
	if err != nil {
		return nil, err
	}
	return ad.rdFromDecision(ctx, decisions)
}

func (ad *AutoDuty) rdsToCleanup(ctx context.Context, execCtx *steps.InstructionCtx) (*types.RequestDecisionTuple, error) {
	decisions, err := ad.cmsdb.GetDecisionsToCleanup(ctx, doneDecisionsFromCtx(execCtx))
	if err != nil {
		return nil, err
	}
	return ad.rdFromDecision(ctx, decisions)
}

func (ad *AutoDuty) rdFromDecision(ctx context.Context, decision models.AutomaticDecision) (*types.RequestDecisionTuple, error) {
	reqs, err := ad.cmsdb.GetRequestsWithDeletedByID(ctx, []int64{decision.RequestID})
	if err != nil {
		return nil, err
	}
	return &types.RequestDecisionTuple{
		R: reqs[decision.RequestID],
		D: decision,
	}, nil
}

func doneDecisionsFromCtx(execCtx *steps.InstructionCtx) []int64 {
	doneRds := execCtx.DoneRDs()
	res := make([]int64, len(doneRds))
	for i, rd := range doneRds {
		res[i] = rd.D.ID
	}
	return res
}
