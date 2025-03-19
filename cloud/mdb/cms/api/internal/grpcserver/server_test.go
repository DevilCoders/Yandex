package grpcserver_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/grpcserver"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func newService(cms cmsdb.Client) *grpcserver.InstanceService {
	return grpcserver.NewInstanceService(cms, nil, nil, nil, nil, nil, nil, nil, grpcserver.DefaultConfig())
}

func TestGetInstanceOperation(t *testing.T) {
	ctx := context.Background()
	opID := "qwe"
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	cms := mocks.NewMockClient(ctrl)
	cms.EXPECT().GetInstanceOperation(ctx, gomock.Any()).Return(models.ManagementInstanceOperation{
		ID:          opID,
		Type:        models.InstanceOperationMove,
		Status:      models.InstanceOperationStatusInProgress,
		Comment:     "testing",
		Author:      "test",
		InstanceID:  "fqdn",
		Explanation: "tbd",
	}, nil)

	s := newService(cms)
	res, err := s.Get(ctx, &api.GetInstanceOperationRequest{
		Id: opID,
	})

	require.NoError(t, err)
	require.Equal(t, opID, res.Id)
	require.Equal(t, api.InstanceOperation_PROCESSING, res.Status)
}

func TestListInstanceOperations(t *testing.T) {
	ctx := context.Background()
	opID := "qwe"
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	cms := mocks.NewMockClient(ctrl)
	cms.EXPECT().ListInstanceOperations(ctx).Return([]models.ManagementInstanceOperation{
		{

			ID:                opID,
			Type:              models.InstanceOperationWhipPrimaryAway,
			Status:            models.InstanceOperationStatusInProgress,
			ExecutedStepNames: []string{"test-step"},
		},
	}, nil)

	s := newService(cms)
	res, err := s.List(ctx, &api.ListInstanceOperationsRequest{})

	require.NoError(t, err)
	require.Len(t, res.Operations, 1)
	op := res.Operations[0]
	require.Equal(t, opID, op.Id)
	require.Equal(t, []api.InstanceOperation_StepName{api.InstanceOperation_STEP_UNKNOWN}, op.ExecutedSteps)
	require.Equal(t, api.InstanceOperation_PROCESSING, op.Status)
}
