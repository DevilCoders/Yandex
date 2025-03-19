package tests

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/grpc"
	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	marketplacestub "a.yandex-team.ru/cloud/mdb/internal/compute/marketplace/stub"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/intapi"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/notifieremulator"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/workeremulation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/functest"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
	pillarsecrets "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/recipe"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/test/yatest"
)

const metaCluster = "metadb"
const metaDatabase = "dbaas_metadb"

type TestContext struct {
	LoadedMT        *models.MaintenanceTaskConfig
	stepsConfig     map[string]models.MaintenanceTaskConfig
	databases       []string
	createdClusters []string
	App             *internal.MaintenanceApp
	intAPI          *intapi.Client
	worker          *workeremulation.Worker
	sender          *notifieremulator.API
	cmsMatcherState expectations.MatcherFullMethod

	GRPCCtx *functest.GRPCContext

	mdb *sqlutil.Cluster
	TC  *godogutil.TestContext
}

func DefaultCmsMatcherByTask() expectations.MatcherFullMethod {
	return expectations.MatcherFullMethod{
		Matchers: map[string]expectations.Matcher{
			"": &expectations.MatcherContains{
				Expected: nil,
				Matcher:  nil,
			},
		},
	}
}

func NewTestContext(tc *godogutil.TestContext) (*TestContext, error) {
	matcher := DefaultCmsMatcherByTask()
	mocker := NewTestCMSGrpcServer(&matcher, 40451)
	cfg := internal.DefaultConfig()
	cfg.MetaDB.User = "mdb_maintenance"
	cfg.MetaDB.DB = metaDatabase
	cfg.MetaDB.Addrs = []string{fmt.Sprintf("%v:%v", metadbhelpers.MustHost(), metadbhelpers.MustPort())}
	cfg.TaskConfigsDirName = yatest.SourcePath("cloud/mdb/mdb-maintenance/configs")
	cfg.Maintainer.AllowClustersWithoutMW = true
	cfg.Maintainer.TaskIDPrefixes = map[string]string{
		"clickhouse_cluster": "ccc",
		"postgresql_cluster": "ppp",
		"greenplum_cluster":  "ggg",
		"mysql_cluster":      "mmm",
	}
	cfg.CMS = grpc.Config{
		Host: mocker.Addr(),
		Transport: grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{Insecure: true},
			Logging: interceptors.LoggingConfig{
				LogRequestBody:  true,
				LogResponseBody: true,
			}}}
	cfg.Holidays.Enabled = false
	cfg.App.Solomon.URL = "placeholder"

	opts := []app.AppOption{
		app.WithConfig(&cfg),
		app.WithLoggerConstructor(app.DefaultToolLoggerConstructor()),
		app.WithMetrics(),
	}

	baseApp, err := app.New(opts...)
	if err != nil {
		return nil, err
	}

	sender := notifieremulator.NewAPI(baseApp.L())

	maintenanceApp, err := internal.NewApp(baseApp, cfg, sender, nil)
	if err != nil {
		return nil, err
	}

	intAPI := intapi.New()
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
	defer cancel()

	if err := ready.Wait(ctx, intAPI, &ready.DefaultErrorTester{}, time.Second); err != nil {
		return nil, err
	}

	mdb, _, err := dbteststeps.NewReadyCluster(metaCluster, metaDatabase)
	if err != nil {
		return nil, err
	}

	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	mockdata := &functest.MockData{}

	logsdbCtx, err := functest.NewLogsDBContext(l)
	if err != nil {
		return nil, err
	}

	if err := pillarsecrets.StartPillarSecretsAPI(functest.AccessServiceStub()); err != nil {
		return nil, err
	}

	licenseService := marketplacestub.LicenseStub(true)

	iapi, _, err := functest.InitInternalAPI(licenseService, mockdata, logsdbCtx.Mock.DB, l)
	if err != nil {
		return nil, err
	}

	grpcCtx, err := functest.NewgRPCContext(iapi.GRPCPort(), l)
	if err != nil {
		return nil, err
	}

	grpcCtx.AuthToken = "rw-token"

	return &TestContext{
		cmsMatcherState: matcher,
		TC:              tc,
		App:             maintenanceApp,
		intAPI:          intAPI,
		mdb:             mdb,
		worker:          workeremulation.New("dummy", mdb),
		sender:          sender,
		GRPCCtx:         grpcCtx,
	}, nil
}
