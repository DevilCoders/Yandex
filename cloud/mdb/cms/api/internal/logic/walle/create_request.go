package walle

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/statemachine"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func dryRunRequest() models.RequestStatus {
	return models.StatusInProcess
}

func (wi *WalleInteractor) CreateRequest(
	ctx context.Context,
	user authentication.Result,
	name string,
	taskID string,
	comment string,
	author string,
	taskType string,
	extra interface{},
	fqnds []string,
	failureType string,
	scenarioInfo models.ScenarioInfo,
	dryRun bool,
) (models.RequestStatus, error) {
	ctx, err := wi.cmsdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return "", err
	}
	defer wi.cmsdb.Rollback(ctx)

	if err := authorize(ctx, user); err != nil {
		return models.StatusInProcess, err
	}
	if dryRun {
		return dryRunRequest(), nil
	}

	previousDecisions, err := wi.cmsdb.GetNotFinishedDecisionsByFQDN(ctx, fqnds)
	if err != nil {
		return "", err
	}
	for _, dec := range previousDecisions {
		rd := &types.RequestDecisionTuple{D: dec}
		if err := wi.sm.TransitToState(
			ctx,
			rd,
			statemachine.Input{
				Action: steps.AfterStepClean,
			},
		); err != nil {
			ctxlog.Error(ctx, wi.log, "there is not finished decision, but we can not move it to clean state",
				log.Int64("decisionID", dec.ID), log.Error(err))
			return "", xerrors.Errorf("close previous decision %d: %w", dec.ID, err)
		}
		ctxlog.Debug(ctx, wi.log, "moved previous decision to cleanup", log.Int64("decisionID", dec.ID))
	}

	createUs := []cmsdb.RequestToCreate{{
		Name:         name,
		ExtID:        taskID,
		Comment:      comment,
		Author:       author,
		RequestType:  taskType,
		Fqnds:        fqnds,
		Extra:        extra,
		FailureType:  failureType,
		ScenarioInfo: scenarioInfo,
	}}
	status, err := wi.cmsdb.CreateRequests(ctx, createUs)
	if err != nil {
		return "", err
	}
	reqs, err := wi.cmsdb.GetRequestsByTaskID(ctx, []string{taskID})
	if err != nil {
		return "", err
	}
	req := reqs[taskID]
	dID, err := wi.cmsdb.CreateDecision(ctx, req.ID, models.DecisionNone, "to be processed")
	wi.log.Debugf("Created decision in CMS id=%d", dID)
	if err != nil {
		return "", err
	}

	err = wi.cmsdb.Commit(ctx)
	if err != nil {
		return status, err
	}
	return status, nil
}
