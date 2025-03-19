package internal

import (
	"context"
	"fmt"
	"os"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery/metadbdiscovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/grpcdiscovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/jugglerbased"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster/mlock"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/duty"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	grpcinstanceclient "a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/grpc"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployapi2 "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	conductor "a.yandex-team.ru/cloud/mdb/internal/conductor/httpapi"
	dbmapi "a.yandex-team.ru/cloud/mdb/internal/dbm/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	jugglerclient "a.yandex-team.ru/cloud/mdb/internal/juggler/http"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	grpcmlockclient "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/grpc"
)

type ExitStatus int

const (
	configErrorExit  ExitStatus = iota + 1
	runtimeErrorExit ExitStatus = iota
)

func exit(status ExitStatus, msg string) {
	fmt.Println(msg)
	os.Exit(int(status))
}

type DutyCLI struct {
	duty *duty.AutoDuty
	app  *app.App
}

func NewDutyCLI() *DutyCLI {
	// * app
	cfg := DefaultConfig()
	baseApp, err := app.New(app.DefaultCronOptions(&cfg, fmt.Sprintf("%s.yaml", AppCfgName))...)
	if err != nil {
		exit(configErrorExit, fmt.Sprintf("cannot make base app, %s", err))
	}

	logger := baseApp.L()

	cmsdb, err := pg.NewCMSDBWithConfigLoad(logger)
	if err != nil {
		exit(configErrorExit, "could not create cmsdb client")
	}

	awaitCtx, cancel := context.WithTimeout(baseApp.ShutdownContext(), 10*time.Second)
	defer cancel()
	err = ready.Wait(awaitCtx, cmsdb, &ready.DefaultErrorTester{Name: "cms database", L: logger}, time.Second)
	if err != nil {
		exit(configErrorExit, "cms cmsdb not ready")
	}

	metadb, err := pg.NewMetaDBWithConfigLoad(logger)
	if err != nil {
		exit(configErrorExit, "could not create cmsdb client")
	}
	awaitCtx, cancel = context.WithTimeout(baseApp.ShutdownContext(), 10*time.Second)
	defer cancel()
	err = ready.Wait(awaitCtx, metadb, &ready.DefaultErrorTester{Name: "metadb", L: logger}, time.Second)
	if err != nil {
		exit(configErrorExit, "metadb not ready")
	}

	// * dbm
	dbm, err := dbmapi.New(cfg.Dbm, logger)
	if err != nil {
		exit(configErrorExit, "could not create dbm client")
	}
	cncl, err := conductor.New(cfg.Conductor, logger)
	if err != nil {
		exit(configErrorExit, "could not create conductor client")
	}

	hlthcl, err := swagger.NewClientTLSFromConfig(cfg.Health, logger)
	if err != nil {
		exit(configErrorExit, "could not create health client")
	}
	jglr, err := jugglerclient.NewClient(cfg.Juggler, logger)
	if err != nil {
		exit(configErrorExit, "could not create juggler client")
	}

	jugglerHealth := jugglerbased.NewJugglerBasedHealthiness(cncl, cfg.Cms.GroupsWhiteL, juggler.NewJugglerReachabilityChecker(jglr, cfg.Cms.Juggler.UnreachableServiceWindowMin), logger)

	var deploy deployapi.Client
	deploy, err = deployapi2.NewFromConfig(cfg.Deploy, logger)
	if err != nil {
		exit(configErrorExit, "could not create deploy api client")
	}

	appCfg := cfg.App
	client, err := grpc.NewTokenServiceClient(
		baseApp.ShutdownContext(),
		appCfg.Environment.Services.Iam.V1.TokenService.Endpoint,
		appCfg.AppName,
		grpcutil.DefaultClientConfig(),
		&grpcutil.PerRPCCredentialsStatic{},
		logger,
	)
	if err != nil {
		exit(configErrorExit, fmt.Sprintf("failed to initialize token service client %v", err))
	}
	creds := client.ServiceAccountCredentials(iam.ServiceAccount{
		ID:    appCfg.ServiceAccount.ID,
		KeyID: appCfg.ServiceAccount.KeyID.Unmask(),
		Token: []byte(appCfg.ServiceAccount.PrivateKey.Unmask()),
	})

	mlockclient, err := grpcmlockclient.NewFromConfig(baseApp.ShutdownContext(), cfg.Mlock, appCfg.AppName, creds, logger)

	if err != nil {
		exit(configErrorExit, fmt.Sprintf("failed to initialize mlock %v", err))
	}

	instanceCLient, err := grpcinstanceclient.NewFromConfig(baseApp.ShutdownContext(), cfg.Instance, "cms-wall-e-autoduty", logger, creds)
	if err != nil {
		exit(configErrorExit, fmt.Sprintf("failed to initialize instance cms client %v", err))
	}
	dom0d := grpcdiscovery.NewDiscovery(instanceCLient)
	dscvr := metadbdiscovery.NewMetaDBBasedDiscovery(metadb)
	locker := mlock.NewLocker(mlockclient, dscvr)

	// init
	autoduty := duty.NewDuty(
		jugglerHealth,
		dom0d,
		logger,
		mlockclient,
		locker,
		appCfg.AppName,
		cmsdb,
		dbm,
		deploy,
		jglr,
		cncl,
		hlthcl,
		metadb,
		cfg.Cms)

	return &DutyCLI{
		duty: &autoduty,
		app:  baseApp,
	}
}

func (d *DutyCLI) Run() {
	err := d.duty.Run(d.app.ShutdownContext())
	d.app.Shutdown()
	if err != nil {
		d.app.L().Errorf("exit because of error: %v", err)
		exit(runtimeErrorExit, fmt.Sprintf("runtime error: %s", err))
	}
}
