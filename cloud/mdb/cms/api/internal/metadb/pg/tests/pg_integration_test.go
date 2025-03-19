package tests_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/pg"
	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
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

	td.intAPI = intAPI
	td.worker = worker
	td.clusterID = op.Metadata.ClusterID
}

func initPG(t *testing.T) *TestData {
	ctx := context.Background()
	logger, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))

	cluster, _, err := dbteststeps.NewReadyCluster("metadb", "dbaas_metadb", sqlutil.WithTracer(tracers.Log(logger)))
	require.NoError(t, err)
	mdb := pg.NewWithCluster(cluster, logger)

	td := &TestData{
		ctx:        ctx,
		mdb:        mdb,
		mdbCluster: cluster,
		l:          logger,
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

	ctx, err := td.mdb.Begin(td.ctx, sqlutil.Primary)
	defer func() {
		err := td.mdb.Rollback(ctx)
		require.NoError(t, err)
	}()
	require.NoError(t, err)

	rev, err := td.mdb.LockCluster(ctx, td.clusterID, "qwe")
	require.NoError(t, err)
	td.l.Debug("revision", log.Int64("revision", rev))

	op, err := td.mdb.CreateTask(ctx, models.CreateTaskArgs{
		TaskID:          "qweasd",
		ClusterID:       td.clusterID,
		FolderID:        1,
		OperationType:   "asd",
		TaskType:        "asd",
		TaskArgs:        map[string]interface{}{"k": 123, "v": "az"},
		Metadata:        nil,
		Auth:            as.Subject{User: &as.UserAccount{ID: "test user"}},
		Hidden:          true,
		SkipIdempotence: true,
		Revision:        rev,
	})
	require.NoError(t, err)

	err = td.mdb.CompleteClusterChange(ctx, td.clusterID, rev)
	require.NoError(t, err)

	err = td.mdb.Commit(ctx)
	require.NoError(t, err)

	ctx2, err := td.mdb.Begin(td.ctx, sqlutil.Primary)
	defer func() {
		err := td.mdb.Rollback(ctx2)
		require.NoError(t, err)
	}()
	require.NoError(t, err)

	op2, err := td.mdb.Task(ctx2, op.OperationID)
	require.NoError(t, err)

	err = td.mdb.Commit(ctx2)
	require.NoError(t, err)

	require.Equal(t, op, op2)
}
