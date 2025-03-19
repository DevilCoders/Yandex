package steps

import (
	"context"
	"fmt"
	"strings"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
)

type fqdnOperationIDPair struct {
	fqdn string
	opID string
}

func (s *CmsInstanceWhipPrimariesStep) GetStepName() string {
	return "request cms instance"
}

type CmsInstanceWhipPrimariesStep struct {
	cmsInstance          instanceclient.InstanceClient
	fqdnOperationIDPairs []fqdnOperationIDPair
	failedFqdns          []string
}

func (s *CmsInstanceWhipPrimariesStep) GetWhipPrimaryOperationInProgress(ctx context.Context, operationID string) (api.InstanceOperation_Status, error) {
	opResult, err := s.cmsInstance.GetInstanceOperation(ctx, operationID)
	if err != nil {
		return api.InstanceOperation_STATUS_UNKNOWN, err
	}

	return opResult.Status, nil
}

func (s *CmsInstanceWhipPrimariesStep) CheckOperations(ctx context.Context, stepState *opmetas.CmsInstanceWhipPrimaryMeta) ([]string, bool, error) {
	failedOperationIDs := []string{}
	isFinished := true
	for fqdn, opID := range stepState.GetOperationIDs() {
		opStatus, err := s.GetWhipPrimaryOperationInProgress(ctx, opID.ID)
		if err != nil {
			isFinished = false
			return nil, isFinished, err
		}

		stepState.SetOperationInfo(fqdn, opID.ID, opStatus)
		if opStatus == api.InstanceOperation_ERROR {
			failedOperationIDs = append(failedOperationIDs, opID.ID)
		}
		if opStatus != api.InstanceOperation_OK {
			isFinished = false
		}
	}
	return failedOperationIDs, isFinished, nil
}

func (s *CmsInstanceWhipPrimariesStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	fqdn := rd.R.MustOneFQDN()

	instancesResponces, err := s.cmsInstance.ResolveInstancesByDom0(ctx, []string{fqdn})
	if err != nil {
		return waitWithErrAndMessage(err, "can not resolve instances")
	}

	stepState := rd.D.OpsLog.CmsInstanceWhipPrimary
	if stepState == nil {
		stepState = opmetas.NewCmsInstanceWhipPrimaryMeta()
		rd.D.OpsLog.CmsInstanceWhipPrimary = stepState
	}
	for _, instance := range instancesResponces.Instance {
		_, ok := stepState.OperationIDs[instance.Fqdn]
		if !ok {
			instanceFullName := fmt.Sprintf("%s:%s", instance.Dom0, instance.Fqdn)

			idem, err := idempotence.New()
			if err != nil {
				return waitWithErrAndMessage(err, "can not create idempotence")
			}
			ictx := idempotence.WithOutgoing(ctx, idem)

			op, err := s.cmsInstance.CreateWhipPrimaryOperation(ictx, instanceFullName, "")
			if err != nil {
				return waitWithErrAndMessage(err, "can not create operation")
			}
			stepState.SetOperationInfo(instance.Fqdn, op.Id, op.GetStatus())
		}
	}

	failedOperations, isFinished, err := s.CheckOperations(ctx, stepState)
	if err != nil {
		return waitWithErrAndMessage(err, "can not get operation")
	}

	if len(failedOperations) > 0 {
		return waitWithErrAndMessage(nil, fmt.Sprintf("Failed whip primary operations %s ", strings.Join(failedOperations, ", ")))
	}
	if isFinished {
		return continueWithMessage("CMS instance whip primary finished")
	} else {
		return waitWithMessage("Waiting CMS instance whip primary operation finished")
	}
}

func NewCmsInstanceWhipPrimariesRequestStep(
	cmsInstance instanceclient.InstanceClient,
) DecisionStep {
	return &CmsInstanceWhipPrimariesStep{
		cmsInstance: cmsInstance,
	}
}
