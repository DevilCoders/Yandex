package instanceclient

import (
	"context"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

//go:generate ../../../../scripts/mockgen.sh InstanceClient

type InstanceClient interface {
	List(ctx context.Context) (*api.ListInstanceOperationsResponse, error)
	CreateMoveInstanceOperation(ctx context.Context, instanceID string, comment string, force bool) (*api.InstanceOperation, error)
	CreateWhipPrimaryOperation(ctx context.Context, instanceID string, comment string) (*api.InstanceOperation, error)
	GetInstanceOperation(ctx context.Context, operationID string) (*api.InstanceOperation, error)
	AlarmOperations(ctx context.Context) (*api.AlarmResponse, error)
	ChangeOperationStatus(ctx context.Context, operationID string, status models.InstanceOperationStatus) error
	ResolveInstancesByDom0(ctx context.Context, dom0s []string) (*api.InstanceListResponce, error)
	Dom0Instances(ctx context.Context, dom0 string) (*api.Dom0InstancesResponse, error)
}

const (
	StepNameWhipMaster     = steps.StepNameWhipMaster
	StepNameCheckIfPrimary = steps.StepNameCheckIfPrimary
)
