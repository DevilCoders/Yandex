package grpcserver

import (
	"time"

	"google.golang.org/protobuf/types/known/timestamppb"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
)

var stepNameMap = map[string]api.InstanceOperation_StepName{
	steps.StepNameWhipMaster:     api.InstanceOperation_WHIP_MASTER,
	steps.StepNameCheckIfPrimary: api.InstanceOperation_CHECK_IF_PRIMARY,
}

func stepNameToGRPC(step string) api.InstanceOperation_StepName {
	if result, ok := stepNameMap[step]; ok {
		return result
	}
	return api.InstanceOperation_STEP_UNKNOWN
}

func stepNamesToGRPC(steps []string) []api.InstanceOperation_StepName {
	result := make([]api.InstanceOperation_StepName, len(steps))
	for ind, s := range steps {
		result[ind] = stepNameToGRPC(s)
	}
	return result
}

func internalOperationToGRPC(op models.ManagementInstanceOperation, l log.Logger) (*api.InstanceOperation, error) {
	createdTime := timestamppb.New(op.CreatedAt)
	if err := createdTime.CheckValid(); err != nil {
		return nil, err
	}
	modifiedTime := timestamppb.New(op.ModifiedAt)
	if err := modifiedTime.CheckValid(); err != nil {
		return nil, err
	}
	resp := &api.InstanceOperation{
		ExecutedSteps: stepNamesToGRPC(op.ExecutedStepNames),
		Id:            op.ID,
		InstanceId:    op.InstanceID,
		CreatedAt:     createdTime,
		ModifiedAt:    modifiedTime,
		CreatedBy:     string(op.Author),
		StatusMessage: op.Explanation,
	}

	switch op.Status {
	case models.InstanceOperationStatusRejected:
		{
			resp.Error = grpcerr.ErrorToGRPC(semerr.FailedPrecondition(op.Explanation), false, l).Proto()
			resp.Status = api.InstanceOperation_ERROR
		}
	case models.InstanceOperationStatusOK:
		resp.Status = api.InstanceOperation_OK
	default:
		{
			resp.Status = api.InstanceOperation_PROCESSING
			resp.DeliveryEstimation = timestamppb.New(time.Now().Add(time.Minute))
			if err := resp.DeliveryEstimation.CheckValid(); err != nil {
				return resp, err
			}
		}
	}

	return resp, nil
}
