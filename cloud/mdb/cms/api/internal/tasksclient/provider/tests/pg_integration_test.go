package tests_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/provider"
	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	legacymeta "a.yandex-team.ru/cloud/mdb/internal/metadb"
	legacymetapg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/intapi"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/workeremulation"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type TestData struct {
	ctx        context.Context
	mdb        metadb.Backend
	mdbCluster *sqlutil.Cluster
	l          log.Logger
	intAPI     *intapi.Client
	worker     *workeremulation.Worker
	legacyMeta legacymeta.MetaDB
	client     tasksclient.Client

	clusterID string
}

func prepareData(t *testing.T, td *TestData) {
	intAPI := intapi.New()
	ctxT, cancel := context.WithTimeout(td.ctx, time.Minute)
	defer cancel()

	err := ready.Wait(ctxT, intAPI, &ready.DefaultErrorTester{}, time.Second)
	require.NoError(t, err)

	worker := workeremulation.New("dummy", td.mdbCluster)

	op, err := intAPI.CreatePGCluster(intapi.CreatePostgresqlClusterRequest{Name: "test_cluster"})
	require.NoError(t, err)

	err = worker.AcquireAndSuccessfullyCompleteTask(td.ctx, op.ID)
	require.NoError(t, err)
	clusterID := op.Metadata.ClusterID

	td.intAPI = intAPI
	td.worker = worker
	td.clusterID = clusterID
}

func initPG(t *testing.T) *TestData {
	ctx := context.Background()
	logger, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))

	cluster, _, err := dbteststeps.NewReadyCluster("metadb", "dbaas_metadb", sqlutil.WithTracer(tracers.Log(logger)))
	require.NoError(t, err)
	mdb := pg.NewWithCluster(cluster, logger)
	legacyMeta := legacymetapg.NewWithCluster(cluster, logger)

	td := &TestData{
		ctx:        ctx,
		mdb:        mdb,
		mdbCluster: cluster,
		l:          logger,
		legacyMeta: legacyMeta,
		client:     provider.NewTasksClient(mdb, legacyMeta),
	}

	prepareData(t, td)

	return td
}

func tearDown(t *testing.T, ctx context.Context, mdb metadb.Backend, cluster *sqlutil.Cluster) {
	require.NoError(t, metadbhelpers.CleanupMetaDB(ctx, cluster.Primary()))
	require.NoError(t, mdb.Close())
}

func TestCreateTaskWorkflow(t *testing.T) {
	td := initPG(t)
	defer tearDown(t, td.ctx, td.mdb, td.mdbCluster)

	ctx := context.Background()
	taskID, err := td.client.CreateMoveInstanceTask(ctx, "myt-1.db.yandex.net", "dom01f.db.yandex.net")
	require.NoError(t, err)

	status, err := td.client.TaskStatus(ctx, taskID)
	require.NoError(t, err)
	require.Equal(t, models.TaskStatusPending, status)

	err = td.worker.AcquireAndSuccessfullyCompleteTask(ctx, taskID)
	require.NoError(t, err)

	status, err = td.client.TaskStatus(ctx, taskID)
	require.NoError(t, err)
	require.Equal(t, models.TaskStatusDone, status)
}
