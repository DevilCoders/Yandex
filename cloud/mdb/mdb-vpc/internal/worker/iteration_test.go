package worker_test

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"golang.yandex/hasql"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	vpcdbmocks "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network"
	networkmocks "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type opMatcher struct {
	origOp models.Operation
}

func (m *opMatcher) Matches(x interface{}) bool {
	op, ok := x.(models.Operation)
	if !ok {
		return false
	}
	return op.ID == m.origOp.ID
}

func (m *opMatcher) String() string {
	return "operation matcher"
}

func generateOperations(length int, err error) []func() (models.Operation, error) {
	res := make([]func() (models.Operation, error), length)
	for i := 0; i < length; i++ {
		if err == nil {
			n := i
			res[i] = func() (models.Operation, error) {
				id := fmt.Sprintf("some-id-%d", n)
				return models.Operation{
					Action:    models.OperationActionCreateVPC,
					ID:        id,
					ProjectID: "p1",
					Provider:  models.ProviderAWS,
					Region:    "eu-central-1",
					Params: &models.CreateNetworkOperationParams{
						NetworkID: id,
					},
				}, nil
			}
		} else {
			res[i] = func() (models.Operation, error) {
				return models.Operation{}, err
			}
		}
	}
	return res
}

func TestWorker_Iteration(t *testing.T) {
	type keyT string
	const key keyT = "number"
	tcs := []struct {
		name       string
		operations []func() (models.Operation, error)
		regions    []string

		maxConcurrentTasks int
	}{
		{
			name: "simple iteration",
			operations: []func() (models.Operation, error){
				func() (models.Operation, error) {
					return models.Operation{
						Action:    models.OperationActionCreateVPC,
						ID:        "asd-qwe",
						ProjectID: "zxc",
						Provider:  models.ProviderAWS,
						Region:    "eu-central-1",
						Params: &models.CreateNetworkOperationParams{
							NetworkID: "asd-qwe-test",
						},
					}, nil
				},
			},
			regions: []string{"eu-central-1"},

			maxConcurrentTasks: 1,
		},
		{
			name: "2 of 3",
			operations: []func() (models.Operation, error){
				func() (models.Operation, error) {
					return models.Operation{
						Action:    models.OperationActionCreateVPC,
						ID:        "will-be-q1",
						ProjectID: "p1",
						Provider:  models.ProviderAWS,
						Region:    "eu-central-1",
						Params: &models.CreateNetworkOperationParams{
							NetworkID: "n1",
						},
					}, nil
				},
				func() (models.Operation, error) {
					return models.Operation{
						Action:    models.OperationActionCreateVPC,
						ID:        "will-be-w2",
						ProjectID: "p2",
						Provider:  models.ProviderAWS,
						Region:    "eu-central-1",
						Params: &models.CreateNetworkOperationParams{
							NetworkID: "n2",
						},
					}, nil
				},
				func() (models.Operation, error) {
					return models.Operation{
						Action:    models.OperationActionCreateVPC,
						ID:        "not-will-be",
						ProjectID: "p3",
						Provider:  models.ProviderAWS,
						Region:    "eu-central-1",
						Params: &models.CreateNetworkOperationParams{
							NetworkID: "n3",
						},
					}, nil
				},
			},
			regions: []string{"eu-central-1"},

			maxConcurrentTasks: 2,
		},
		{
			name: "3 of 1",
			operations: []func() (models.Operation, error){
				func() (models.Operation, error) {
					return models.Operation{
						Action:    models.OperationActionCreateVPC,
						ID:        "will-be-q1",
						ProjectID: "p1",
						Provider:  models.ProviderAWS,
						Region:    "eu-central-1",
						Params: &models.CreateNetworkOperationParams{
							NetworkID: "n1",
						},
					}, nil
				},
				func() (models.Operation, error) {
					return models.Operation{}, semerr.NotFound("")
				},
				func() (models.Operation, error) {
					return models.Operation{}, semerr.NotFound("")
				},
			},
			regions: []string{"eu-central-1"},

			maxConcurrentTasks: 3,
		},
		{
			name:       "many gorutines",
			operations: append(generateOperations(100, nil), generateOperations(100, semerr.NotFound(""))...),
			regions:    []string{"eu-central-1"},

			maxConcurrentTasks: 200,
		},
	}

	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			db := vpcdbmocks.NewMockVPCDB(ctrl)
			db.EXPECT().IsReady(ctx).Return(nil).AnyTimes()
			for i := 0; i < len(tc.operations); i++ {
				n := i
				op, err := tc.operations[n]()
				db.EXPECT().Begin(gomock.Any(), sqlutil.Primary).DoAndReturn(
					func(ctx context.Context, ns hasql.NodeStateCriteria) (context.Context, error) {
						opCtx := ctxlog.WithFields(ctx, log.Int("test run", n))
						opCtx = context.WithValue(opCtx, key, n)
						return opCtx, nil
					},
				).AnyTimes()
				db.EXPECT().OperationToProcess(gomock.Any()).Return(op, err).AnyTimes()
				db.EXPECT().UpdateOperationFields(gomock.Any(), &opMatcher{op}).DoAndReturn(
					func(ctx context.Context, newOp models.Operation) error {
						require.Equal(t, op.ID, newOp.ID)
						require.NotEqual(t, op.Status, newOp.Status)
						require.NotEqual(t, op.StartTime, newOp.StartTime)
						return nil
					},
				).AnyTimes()
				db.EXPECT().Commit(gomock.Any()).Return(nil).AnyTimes()
				db.EXPECT().Rollback(gomock.Any()).Return(nil).AnyTimes()
			}

			networkmock := networkmocks.NewMockService(gomock.NewController(t))
			for i := 0; i < tc.maxConcurrentTasks; i++ {
				op, _ := tc.operations[i]()
				networkmock.EXPECT().CreateNetwork(gomock.Any(), &opMatcher{op}).Return(nil).AnyTimes()
			}
			awsNetworkProviders := make(map[string]network.Service)
			for _, region := range tc.regions {
				awsNetworkProviders[region] = networkmock
			}

			w := worker.NewCustomWorker(l, db, tc.maxConcurrentTasks, awsNetworkProviders)
			w.Iteration(ctx)
		})
	}
}
