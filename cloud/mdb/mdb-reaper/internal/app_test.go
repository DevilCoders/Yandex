package internal

import (
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"google.golang.org/genproto/googleapis/rpc/status"

	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	dcv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	resmocks "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/mocks"
	chmocks "a.yandex-team.ru/cloud/mdb/internal/mocks/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	kfmocks "a.yandex-team.ru/cloud/mdb/internal/mocks/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-reaper/internal/metadb"
	metamocks "a.yandex-team.ru/cloud/mdb/mdb-reaper/internal/metadb/mocks"
)

func newApp(t *testing.T, conf Config) (*App, *metamocks.MockMetaDB, *chmocks.MockClusterServiceClient, *chmocks.MockOperationServiceClient, *kfmocks.MockClusterServiceClient, *kfmocks.MockOperationServiceClient, *resmocks.MockClient) {
	baseOpts := []app.AppOption{
		app.WithConfig(&conf),
		app.WithLoggerConstructor(app.DefaultToolLoggerConstructor()),
	}
	baseApp, err := app.New(baseOpts...)
	require.NoError(t, err)

	ctrl := gomock.NewController(t)

	meta := metamocks.NewMockMetaDB(ctrl)
	chClusterSvc := chmocks.NewMockClusterServiceClient(ctrl)
	chOperationSvc := chmocks.NewMockOperationServiceClient(ctrl)
	kfClusterSvc := kfmocks.NewMockClusterServiceClient(ctrl)
	kfOperationSvc := kfmocks.NewMockOperationServiceClient(ctrl)
	resManager := resmocks.NewMockClient(ctrl)

	opts := []AppOption{
		WithConfig(conf),
		WithMetaDB(meta),
		WithClickhouseSvc(chClusterSvc, chOperationSvc),
		WithKafkaSvc(kfClusterSvc, kfOperationSvc),
		WithResourceManager(resManager),
	}
	a, err := NewApp(baseApp, opts...)
	require.NoError(t, err)
	return a, meta, chClusterSvc, chOperationSvc, kfClusterSvc, kfOperationSvc, resManager
}

func TestEmptyResult(t *testing.T) {
	a, meta, _, _, _, _, _ := newApp(t, DefaultConfig())
	meta.EXPECT().IsReady(gomock.Any()).Return(nil)
	meta.EXPECT().CloudsWithRunningClusters(gomock.Any(), false).Return(metadb.ClusterIDsByCloudID{}, nil)
	require.NoError(t, a.Reap())
}

func TestNonBlockedCloud(t *testing.T) {
	a, meta, _, _, _, _, resMan := newApp(t, DefaultConfig())
	meta.EXPECT().IsReady(gomock.Any()).Return(nil)
	clouds := metadb.ClusterIDsByCloudID{
		"cloud1": metadb.ClusterIDs{"cid1"},
	}
	meta.EXPECT().CloudsWithRunningClusters(gomock.Any(), false).Return(clouds, nil)
	cloud := resmanager.Cloud{CloudID: "cloud1", Name: "cloud one", Status: resmanager.CloudStatusActive}
	resMan.EXPECT().Cloud(gomock.Any(), "cloud1").Return(cloud, nil)
	require.NoError(t, a.Reap())
}

func TestOneBlockedClickHouseCluster(t *testing.T) {
	a, meta, chClusterSvc, chOperationSvc, _, _, resMan := newApp(t, DefaultConfig())
	meta.EXPECT().IsReady(gomock.Any()).Return(nil)
	clouds := metadb.ClusterIDsByCloudID{
		"cloud1": metadb.ClusterIDs{"cid1"},
	}
	meta.EXPECT().CloudsWithRunningClusters(gomock.Any(), false).Return(clouds, nil)
	cloud := resmanager.Cloud{CloudID: "cloud1", Name: "cloud one", Status: resmanager.CloudStatusBlocked}
	resMan.EXPECT().Cloud(gomock.Any(), "cloud1").Return(cloud, nil)
	clusters := []metadb.Cluster{
		{ID: "cid1", Name: "ch cluster", Type: metadb.ClickhouseCluster, Environment: "qa"},
	}
	meta.EXPECT().Clusters(gomock.Any(), []string{"cid1"}).Return(clusters, nil)
	chClusterSvc.EXPECT().Stop(gomock.Any(), &chv1.StopClusterRequest{ClusterId: "cid1"}, gomock.Any()).Return(&chv1.StopClusterResponse{OperationId: "op1"}, nil)
	chOperationSvc.EXPECT().Get(gomock.Any(), &chv1.GetOperationRequest{OperationId: "op1"}, gomock.Any()).Return(&dcv1.Operation{Id: "op1", Status: dcv1.Operation_STATUS_DONE}, nil)
	require.NoError(t, a.Reap())
}

func TestOneBlockedClickHouseClusterWithLongStop(t *testing.T) {
	a, meta, chClusterSvc, chOperationSvc, _, _, resMan := newApp(t, DefaultConfig())
	meta.EXPECT().IsReady(gomock.Any()).Return(nil)
	clouds := metadb.ClusterIDsByCloudID{
		"cloud1": metadb.ClusterIDs{"cid1"},
	}
	meta.EXPECT().CloudsWithRunningClusters(gomock.Any(), false).Return(clouds, nil)
	cloud := resmanager.Cloud{CloudID: "cloud1", Name: "cloud one", Status: resmanager.CloudStatusBlocked}
	resMan.EXPECT().Cloud(gomock.Any(), "cloud1").Return(cloud, nil)
	clusters := []metadb.Cluster{
		{ID: "cid1", Name: "ch cluster", Type: metadb.ClickhouseCluster, Environment: "qa"},
	}
	meta.EXPECT().Clusters(gomock.Any(), []string{"cid1"}).Return(clusters, nil)
	chClusterSvc.EXPECT().Stop(gomock.Any(), &chv1.StopClusterRequest{ClusterId: "cid1"}, gomock.Any()).Return(&chv1.StopClusterResponse{OperationId: "op1"}, nil)
	chOperationSvc.EXPECT().Get(gomock.Any(), &chv1.GetOperationRequest{OperationId: "op1"}, gomock.Any()).Return(&dcv1.Operation{Id: "op1", Status: dcv1.Operation_STATUS_PENDING}, nil)
	chOperationSvc.EXPECT().Get(gomock.Any(), &chv1.GetOperationRequest{OperationId: "op1"}, gomock.Any()).Return(&dcv1.Operation{Id: "op1", Status: dcv1.Operation_STATUS_PENDING}, nil)
	chOperationSvc.EXPECT().Get(gomock.Any(), &chv1.GetOperationRequest{OperationId: "op1"}, gomock.Any()).Return(&dcv1.Operation{Id: "op1", Status: dcv1.Operation_STATUS_DONE}, nil)
	require.NoError(t, a.Reap())
}

func TestOneBlockedBothCluster(t *testing.T) {
	a, meta, chClusterSvc, chOperationSvc, kfClusterSvc, kfOperationSvc, resMan := newApp(t, DefaultConfig())
	meta.EXPECT().IsReady(gomock.Any()).Return(nil)
	clouds := metadb.ClusterIDsByCloudID{
		"cloud1": metadb.ClusterIDs{"cid1", "cid2"},
	}
	meta.EXPECT().CloudsWithRunningClusters(gomock.Any(), false).Return(clouds, nil)
	cloud := resmanager.Cloud{CloudID: "cloud1", Name: "cloud one", Status: resmanager.CloudStatusBlocked}
	resMan.EXPECT().Cloud(gomock.Any(), "cloud1").Return(cloud, nil)
	clusters := []metadb.Cluster{
		{ID: "cid1", Name: "ch cluster", Type: metadb.ClickhouseCluster, Environment: "qa"},
		{ID: "cid2", Name: "kf cluster", Type: metadb.KafkaCluster, Environment: "prod]"},
	}
	meta.EXPECT().Clusters(gomock.Any(), []string{"cid1", "cid2"}).Return(clusters, nil)
	chClusterSvc.EXPECT().Stop(gomock.Any(), &chv1.StopClusterRequest{ClusterId: "cid1"}, gomock.Any()).Return(&chv1.StopClusterResponse{OperationId: "op1"}, nil)
	chOperationSvc.EXPECT().Get(gomock.Any(), &chv1.GetOperationRequest{OperationId: "op1"}, gomock.Any()).Return(&dcv1.Operation{Id: "op1", Status: dcv1.Operation_STATUS_DONE}, nil)
	kfClusterSvc.EXPECT().Stop(gomock.Any(), &kfv1.StopClusterRequest{ClusterId: "cid2"}, gomock.Any()).Return(&kfv1.StopClusterResponse{OperationId: "op2"}, nil)
	kfOperationSvc.EXPECT().Get(gomock.Any(), &kfv1.GetOperationRequest{OperationId: "op2"}, gomock.Any()).Return(&dcv1.Operation{Id: "op2", Status: dcv1.Operation_STATUS_DONE}, nil)
	require.NoError(t, a.Reap())
}

func TestOneBlockedClickHouseClusterWithFailedStop(t *testing.T) {
	a, meta, chClusterSvc, chOperationSvc, _, _, resMan := newApp(t, DefaultConfig())
	meta.EXPECT().IsReady(gomock.Any()).Return(nil)
	clouds := metadb.ClusterIDsByCloudID{
		"cloud1": metadb.ClusterIDs{"cid1"},
	}
	meta.EXPECT().CloudsWithRunningClusters(gomock.Any(), false).Return(clouds, nil)
	cloud := resmanager.Cloud{CloudID: "cloud1", Name: "cloud one", Status: resmanager.CloudStatusBlocked}
	resMan.EXPECT().Cloud(gomock.Any(), "cloud1").Return(cloud, nil)
	clusters := []metadb.Cluster{
		{ID: "cid1", Name: "ch cluster", Type: metadb.ClickhouseCluster, Environment: "qa"},
	}
	meta.EXPECT().Clusters(gomock.Any(), []string{"cid1"}).Return(clusters, nil)
	chClusterSvc.EXPECT().Stop(gomock.Any(), &chv1.StopClusterRequest{ClusterId: "cid1"}, gomock.Any()).Return(&chv1.StopClusterResponse{OperationId: "op1"}, nil)
	chOperationSvc.EXPECT().Get(gomock.Any(), &chv1.GetOperationRequest{OperationId: "op1"}, gomock.Any()).Return(&dcv1.Operation{Id: "op1", Status: dcv1.Operation_STATUS_PENDING}, nil)
	chOperationSvc.EXPECT().Get(gomock.Any(), &chv1.GetOperationRequest{OperationId: "op1"}, gomock.Any()).Return(&dcv1.Operation{Id: "op1", Status: dcv1.Operation_STATUS_DONE, Error: &status.Status{Code: 5, Message: "Failed"}}, nil)
	require.Error(t, a.Reap())
}

func TestPurgePrevented(t *testing.T) {
	conf := DefaultConfig()
	conf.StopAllUnused = true
	a, meta, _, _, _, _, _ := newApp(t, conf)
	meta.EXPECT().IsReady(gomock.Any()).Return(nil)
	meta.EXPECT().CloudsWithRunningClusters(gomock.Any(), true).Return(nil, nil)
	require.NoError(t, a.Reap())
}

func TestPurgeNew(t *testing.T) {
	conf := DefaultConfig()
	conf.StopAllUnused = true
	a, meta, chClusterSvc, chOperationSvc, _, _, _ := newApp(t, conf)
	meta.EXPECT().IsReady(gomock.Any()).Return(nil)
	clouds := metadb.ClusterIDsByCloudID{
		"cloud1": metadb.ClusterIDs{"cid1", "cid2"},
	}
	meta.EXPECT().CloudsWithRunningClusters(gomock.Any(), true).Return(clouds, nil)
	// If StopAllUnused is set to true, we do not check a cloud block status.
	clusters := []metadb.Cluster{
		{ID: "cid1", Name: "new cluster", Type: metadb.ClickhouseCluster, Environment: "qa", LastActionAt: time.Now().Add(-time.Hour)},
		{ID: "cid2", Name: "old cluster", Type: metadb.ClickhouseCluster, Environment: "qa", LastActionAt: time.Now().Add(-24 * 15 * time.Hour)},
	}
	meta.EXPECT().Clusters(gomock.Any(), []string{"cid1", "cid2"}).Return(clusters, nil)
	chClusterSvc.EXPECT().Stop(gomock.Any(), &chv1.StopClusterRequest{ClusterId: "cid2"}, gomock.Any()).Return(&chv1.StopClusterResponse{OperationId: "op1"}, nil)
	chOperationSvc.EXPECT().Get(gomock.Any(), &chv1.GetOperationRequest{OperationId: "op1"}, gomock.Any()).Return(&dcv1.Operation{Id: "op1", Status: dcv1.Operation_STATUS_DONE}, nil)
	require.NoError(t, a.Reap())
}
