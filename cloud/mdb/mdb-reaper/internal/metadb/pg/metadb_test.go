package pg

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/intapi"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/workeremulation"
	"a.yandex-team.ru/cloud/mdb/mdb-reaper/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type TestData struct {
	ctx        context.Context
	mdb        metadb.MetaDB
	mdbCluster *sqlutil.Cluster
	l          log.Logger
	intAPI     *intapi.Client
	worker     *workeremulation.Worker

	clusterID string
}

func prepareData(t *testing.T, td *TestData) {
	prepareDataWithName(t, td, "test_cluster")
}

func prepareDataWithName(t *testing.T, td *TestData, name string) {
	intAPI := intapi.New()
	ctxT, cancel := context.WithTimeout(td.ctx, time.Minute)
	defer cancel()

	err := ready.Wait(ctxT, intAPI, &ready.DefaultErrorTester{}, time.Second)
	require.NoError(t, err)

	worker := workeremulation.New("dummy", td.mdbCluster)

	op, err := intAPI.CreatePGCluster(intapi.CreatePostgresqlClusterRequest{
		Name:             name,
		ResourcePresetID: "s1.compute.1",
		DiskTypeID:       "network-ssd",
		NetworkID:        "network1",
		Zones:            []string{"myt"},
	})
	require.NoError(t, err)

	err = worker.AcquireAndSuccessfullyCompleteTask(td.ctx, op.ID)
	require.NoError(t, err)

	td.intAPI = intAPI
	td.worker = worker
	td.clusterID = op.Metadata.ClusterID
}

func initPG(t *testing.T) *TestData {
	ctx := context.Background()
	logger, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))

	cluster, _, err := dbteststeps.NewReadyCluster("metadb", "dbaas_metadb", sqlutil.WithTracer(tracers.Log(logger)))
	require.NoError(t, err)
	mdb := NewWithCluster(cluster, logger)

	td := &TestData{
		ctx:        ctx,
		mdb:        mdb,
		mdbCluster: cluster,
		l:          logger,
	}

	return td
}

func tearDown(t *testing.T, ctx context.Context, mdb metadb.MetaDB, cluster *sqlutil.Cluster) {
	require.NoError(t, metadbhelpers.CleanupMetaDB(ctx, cluster.Primary()))
	require.NoError(t, mdb.Close())
}

func TestCloudsAndClusters(t *testing.T) {
	td := initPG(t)
	prepareData(t, td)

	defer tearDown(t, td.ctx, td.mdb, td.mdbCluster)
	testStartAt := time.Now()

	clouds, err := td.mdb.CloudsWithRunningClusters(td.ctx, false)
	require.NoError(t, err)
	require.Len(t, clouds, 1)
	require.Contains(t, clouds, "cloud1")
	require.ElementsMatch(t, []string{td.clusterID}, clouds["cloud1"])

	clusters, err := td.mdb.Clusters(td.ctx, metadb.ClusterIDs{td.clusterID})
	require.NoError(t, err)
	require.Len(t, clusters, 1)
	require.Equal(t, clusters[0].ID, td.clusterID)
	require.Greater(t, testStartAt, clusters[0].LastActionAt)

	op, err := td.intAPI.StopPGCluster(td.clusterID)
	require.NoError(t, err)

	err = td.worker.AcquireAndSuccessfullyCompleteTask(td.ctx, op.ID)
	require.NoError(t, err)

	clouds, err = td.mdb.CloudsWithRunningClusters(td.ctx, false)
	require.NoError(t, err)
	require.Empty(t, clouds)

	clusters, err = td.mdb.Clusters(td.ctx, metadb.ClusterIDs{td.clusterID})
	require.NoError(t, err)
	require.Len(t, clusters, 1)
	require.Equal(t, clusters[0].ID, td.clusterID)
	require.Greater(t, clusters[0].LastActionAt, testStartAt)
}

func TestExcludeTag(t *testing.T) {
	td := initPG(t)
	prepareDataWithName(t, td, "test_cluster_mdb-auto-purge-off")

	defer tearDown(t, td.ctx, td.mdb, td.mdbCluster)
	testStartAt := time.Now()

	clouds, err := td.mdb.CloudsWithRunningClusters(td.ctx, true)
	require.NoError(t, err)
	require.Empty(t, clouds)

	clouds, err = td.mdb.CloudsWithRunningClusters(td.ctx, false)
	require.NoError(t, err)
	require.Len(t, clouds, 1)
	require.Contains(t, clouds, "cloud1")
	require.ElementsMatch(t, []string{td.clusterID}, clouds["cloud1"])

	clusters, err := td.mdb.Clusters(td.ctx, metadb.ClusterIDs{td.clusterID})
	require.NoError(t, err)
	require.Len(t, clusters, 1)
	require.Equal(t, clusters[0].ID, td.clusterID)
	require.Greater(t, testStartAt, clusters[0].LastActionAt)

	op, err := td.intAPI.StopPGCluster(td.clusterID)
	require.NoError(t, err)

	err = td.worker.AcquireAndSuccessfullyCompleteTask(td.ctx, op.ID)
	require.NoError(t, err)

	clouds, err = td.mdb.CloudsWithRunningClusters(td.ctx, false)
	require.NoError(t, err)
	require.Empty(t, clouds)

	clusters, err = td.mdb.Clusters(td.ctx, metadb.ClusterIDs{td.clusterID})
	require.NoError(t, err)
	require.Len(t, clusters, 1)
	require.Equal(t, clusters[0].ID, td.clusterID)
	require.Greater(t, clusters[0].LastActionAt, testStartAt)
}
