package tests

import (
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"os"
	"path"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/alicebob/miniredis/v2"
	"github.com/go-openapi/loads"
	"github.com/go-openapi/runtime/middleware"
	"github.com/golang/mock/gomock"

	apihelpers "a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/recipe/helpers"
	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/swagger"
	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	leaderelectormocks "a.yandex-team.ru/cloud/mdb/internal/leaderelection/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbpg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/intapi"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/workeremulation"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore/redis"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore"
	ssmocks "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const metaCluster = "metadb"
const metaDatabase = "dbaas_metadb"
const datastoreName = "miniredis"

type TestContext struct {
	intAPI *intapi.Client

	mdb     *sqlutil.Cluster
	minired *miniredis.Miniredis
	TC      *godogutil.TestContext
	worker  *workeremulation.Worker
	app     *internal.App
	api     *operations.MdbHealthAPI
	handler http.Handler
	pk      *crypto.PrivateKey
	hs      *healthstore.Store

	lastResponse *httptest.ResponseRecorder
}

func NewTestContext(tc *godogutil.TestContext, minired *miniredis.Miniredis) (*TestContext, error) {
	intAPI := intapi.New()
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
	defer cancel()

	if err := ready.Wait(ctx, intAPI, &ready.DefaultErrorTester{}, time.Second); err != nil {
		return nil, err
	}

	mdbCluster, _, err := dbteststeps.NewReadyCluster(metaCluster, metaDatabase)
	if err != nil {
		return nil, err
	}

	swaggerSpec, err := loads.Embedded(restapi.SwaggerJSON, restapi.FlatSwaggerJSON)
	if err != nil {
		return nil, err
	}
	api := operations.NewMdbHealthAPI(swaggerSpec)

	datastore.RegisterBackend(datastoreName, func(logger log.Logger) (datastore.Backend, error) {
		redisCfg := redis.DefaultConfig()
		redisCfg.Addrs = []string{minired.Addr()}
		minired.Select(redisCfg.DB)
		return redis.New(logger, redisCfg), nil
	})

	pk, err := crypto.GeneratePrivateKey()
	if err != nil {
		return nil, err
	}

	a, hs := newApp(api.Context(), mdbCluster, pk.GetPublicKey().EncodeToPKCS1())

	handler := restapi.ConfigureAPIWithApp(api, a)

	return &TestContext{
		TC:      tc,
		intAPI:  intAPI,
		mdb:     mdbCluster,
		app:     a,
		api:     api,
		worker:  workeremulation.New("dummy", mdbCluster),
		handler: handler,
		minired: minired,
		pk:      pk,
		hs:      hs,
	}, nil
}

func newApp(mdwCtx *middleware.Context, mdbCluster *sqlutil.Cluster, keyOverride []byte) (*internal.App, *healthstore.Store) {
	cfg := internal.DefaultConfig()

	opts := []app.AppOption{
		app.WithConfig(&cfg),
		app.WithLoggerConstructor(app.DefaultToolLoggerConstructor()),
		app.WithReadyChecker(),
	}

	baseApp, err := swagger.New(mdwCtx, opts...)
	if err != nil {
		panic(err)
	}

	ctrl := gomock.NewController(baseApp.L())

	ds, err := datastore.Open(datastoreName, baseApp.L())
	if err != nil {
		panic(err)
	}
	ss := ssmocks.NewMockBackend(ctrl)
	le := leaderelectormocks.NewMockLeaderElector(ctrl)

	mdb := metadbpg.NewWithCluster(mdbCluster, baseApp.L())

	hsCfg := healthstore.DefaultConfig()
	hsCfg.MaxCycles = 1
	hsCfg.MaxWaitTime = encodingutil.FromDuration(time.Millisecond)
	hs := healthstore.NewStore([]healthstore.Backend{ds}, hsCfg, baseApp.L())

	gwConfig := core.DefaultConfig()
	gwConfig.UpdTopologyTimeout = 0
	gw := core.NewGeneralWard(baseApp.ShutdownContext(), baseApp.L(), gwConfig, ds, hs, ss, mdb, keyOverride, false, le)

	a := &internal.App{
		App: baseApp,
		Cfg: cfg,
		GW:  gw,
	}
	ss.EXPECT().IsReady(gomock.Any()).Return(nil)
	ss.EXPECT().LoadClusterSecret(gomock.Any(), gomock.Any()).Return(nil, secretsstore.ErrSecretNotFound).AnyTimes()
	le.EXPECT().IsLeader(gomock.Any()).Return(true).AnyTimes()
	gw.RegisterReadyChecker(baseApp.ReadyCheckAggregator())

	return a, hs
}

func (tctx *TestContext) RegisterSteps(s *godog.Suite) {
	// REST steps
	s.Step("we GET \"([^\"]*)\"$", tctx.weGet)
	s.Step("we POST \"([^\"]*)\"$", tctx.wePost)
	s.Step("we PUT \"([^\"]*)\"$", tctx.wePut)
	s.Step("we successfully PUT \"([^\"]*)\"$", tctx.weSuccessfullyPut)
	s.Step(`^we get response with status (\d+)$`, tctx.weGetResponseWithStatus)
	s.Step(`^we get response with content$`, tctx.weGetResponseWithContent)
	s.Step(`^we get a response that contains$`, tctx.weGetAResponseThatContains)

	// Application steps
	s.Step(`^force processing for cluster type "([^"]*)"$`, tctx.forceProcessing)

	// Cluster steps
	s.Step(`^create postgres cluster with name "([^"]*)"$`, tctx.postgresClusterWithName)
	s.Step(`^we in "([^"]*)" create postgres cluster with name "([^"]*)" using "([^"]*)" token$`, tctx.weInCreatePostgresClusterWithNameUsingToken)
	s.Step(`^create mysql cluster with name "([^"]*)"$`, tctx.mysqlClusterWithName)

	s.BeforeScenario(tctx.BeforeScenario)
	s.AfterScenario(dbteststeps.DumpOnErrorHook(tctx.TC, tctx.mdb.Primary(), "metadb", "dbaas"))
	s.AfterScenario(tctx.dumpRedisOnErrorHook("redis"))
}

func (tctx *TestContext) BeforeScenario(interface{}) {
	if err := metadbhelpers.CleanupMetaDB(tctx.TC.Context(), tctx.mdb.Primary()); err != nil {
		panic(err)
	}
	if err := os.RemoveAll(apihelpers.MustTmpRootPath("")); err != nil {
		panic(fmt.Sprintf("failed to cleanup tmp root dir: %s", err))
	}
	tctx.minired.FlushAll()
}

func (tctx *TestContext) forceProcessing(ct string) error {
	tctx.app.GW.ForceProcessing(tctx.TC.Context(), metadb.ClusterType(ct), time.Now())
	return nil
}

func (tctx *TestContext) dumpRedisOnErrorHook(toDir string) func(interface{}, error) {
	dumpToPath := func() string {
		names := tctx.TC.LastExecutedNames()
		dumpDirPath := path.Join(toDir, names.FeatureName+" "+names.ScenarioName)
		return godogutil.TestOutputPath(dumpDirPath)
	}

	return func(_ interface{}, err error) {
		if err == nil || xerrors.Is(err, godog.ErrPending) {
			return
		}
		dumpDirPath := dumpToPath()
		if err := os.MkdirAll(dumpDirPath, 0755); err != nil {
			fmt.Printf("fail create dir: %q for database dump: %s\n", dumpDirPath, err)
			return
		}
		filePath := path.Join(dumpDirPath, "dump.txt")
		dumpErr := ioutil.WriteFile(filePath, []byte(tctx.minired.Dump()), 0644)
		if dumpErr != nil {
			fmt.Println(dumpErr)
		}
	}
}
