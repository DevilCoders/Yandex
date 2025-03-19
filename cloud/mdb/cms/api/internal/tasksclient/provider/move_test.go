package provider_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	internalmetadbmocks "a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/provider"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

func TestTasksClient_CreateMoveInstanceTask(t *testing.T) {
	type input struct {
		host         metadb.Host
		cluster      metadb.ClusterInfo
		clusterHosts []metadb.Host
		revision     int64
		operationID  string
	}
	testCases := []struct {
		name         string
		expectedArgs models.CreateTaskArgs
		input        input
	}{
		{
			name: "postgresql_cluster_online_resetup",
			input: input{
				host: metadb.Host{
					FQDN:         "fqdn1",
					ClusterID:    "cluster1",
					SubClusterID: "subcid1",
				},
				cluster: metadb.ClusterInfo{
					CType:    metadb.PostgresqlCluster,
					FolderID: 42,
					Status:   metadb.ClusterStatusRunning,
				},
				clusterHosts: []metadb.Host{
					{},
					{},
				},
				revision:    4,
				operationID: "qwerty",
			},
			expectedArgs: models.CreateTaskArgs{
				ClusterID:     "cluster1",
				FolderID:      42,
				OperationType: "postgresql_cluster_online_resetup",
				TaskType:      "postgresql_cluster_online_resetup",
				TaskArgs: map[string]interface{}{
					"fqdn":                  "fqdn1",
					"resetup_action":        provider.ResetupActionReadd,
					"preserve_if_possible":  false,
					"ignore_hosts":          make([]string, 0),
					"lock_is_already_taken": true,
					"try_save_disks":        false,
					"cid":                   "cluster1",
					"resetup_from":          "dom0",
				},
				Auth:            as.Subject{Service: &as.ServiceAccount{ID: "yc.mdb.cms"}},
				Hidden:          true,
				SkipIdempotence: true,
				Revision:        4,
				Timeout:         optional.NewDuration(24 * 4 * time.Hour),
			},
		},
		{
			name: "clickhouse_cluster_offline_resetup one leg",
			input: input{
				host: metadb.Host{
					FQDN:         "fqdn2",
					ClusterID:    "cluster2",
					SubClusterID: "subcid2",
				},
				cluster: metadb.ClusterInfo{
					CType:    metadb.ClickhouseCluster,
					FolderID: 21,
					Status:   metadb.ClusterStatusStopped,
				},
				clusterHosts: []metadb.Host{
					{
						FQDN: "fqdn2",
					},
				},
				revision:    2,
				operationID: "qwerty",
			},
			expectedArgs: models.CreateTaskArgs{
				ClusterID:     "cluster2",
				FolderID:      21,
				OperationType: "clickhouse_cluster_offline_resetup",
				TaskType:      "clickhouse_cluster_offline_resetup",
				TaskArgs: map[string]interface{}{
					"fqdn":                  "fqdn2",
					"resetup_action":        provider.ResetupActionRestore,
					"preserve_if_possible":  false,
					"ignore_hosts":          make([]string, 0),
					"lock_is_already_taken": true,
					"try_save_disks":        false,
					"cid":                   "cluster2",
					"resetup_from":          "dom0",
				},
				Auth:            as.Subject{Service: &as.ServiceAccount{ID: "yc.mdb.cms"}},
				Hidden:          true,
				SkipIdempotence: true,
				Revision:        2,
				Timeout:         optional.NewDuration(24 * 4 * time.Hour),
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()

			var generatedArgs models.CreateTaskArgs
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			intMeta := internalmetadbmocks.NewMockBackend(ctrl)
			meta := metadbmocks.NewMockMetaDB(ctrl)

			txCtx, cancel := context.WithCancel(ctx)
			defer cancel()
			intMeta.EXPECT().Begin(ctx, sqlutil.Primary).Times(1).Return(txCtx, nil)
			intMeta.EXPECT().Rollback(txCtx).Times(1)
			intMeta.EXPECT().LockCluster(txCtx, tc.input.host.ClusterID, "").Times(1).Return(tc.input.revision, nil)
			intMeta.EXPECT().CreateTask(txCtx, gomock.Any()).Times(1).DoAndReturn(
				func(_ context.Context, args models.CreateTaskArgs) (models.Operation, error) {
					generatedArgs = args
					return models.Operation{
						OperationID: tc.input.operationID,
					}, nil
				})
			intMeta.EXPECT().CompleteClusterChange(txCtx, tc.input.host.ClusterID, tc.input.revision).Times(1).Return(nil)
			intMeta.EXPECT().Commit(txCtx).Times(1).Return(nil)

			meta.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
			meta.EXPECT().Commit(gomock.Any()).Return(nil)
			meta.EXPECT().Rollback(gomock.Any()).Return(nil)
			meta.EXPECT().GetHostByFQDN(gomock.Any(), tc.input.host.FQDN).Return(
				tc.input.host, nil).Times(2)

			meta.EXPECT().ClusterInfo(gomock.Any(), tc.input.host.ClusterID).Return(
				tc.input.cluster, nil)

			meta.EXPECT().GetHostsBySubcid(gomock.Any(), tc.input.host.SubClusterID).Return(
				tc.input.clusterHosts, nil)

			client := provider.NewTasksClient(intMeta, meta)

			_, err := client.CreateMoveInstanceTask(ctx, tc.input.host.FQDN, "dom0")
			require.NoError(t, err)
			require.NoError(t, generatedArgs.Validate())

			require.NotEmpty(t, generatedArgs.TaskID)
			generatedArgs.TaskID = ""
			require.Equal(t, tc.expectedArgs, generatedArgs)
		})
	}
}
