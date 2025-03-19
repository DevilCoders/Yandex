package duty

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery/metadbdiscovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster/mlock"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/duty/config"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/workflow"
	internalpgmeta "a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/shipments"
	shipmentsprovider "a.yandex-team.ru/cloud/mdb/cms/api/internal/shipments/provider"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient"
	tasksclientprovider "a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/provider"
	deployapiint "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployapi "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	fqdnlib "a.yandex-team.ru/cloud/mdb/internal/fqdn"
	fqdn "a.yandex-team.ru/cloud/mdb/internal/fqdn/impl"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglerclient "a.yandex-team.ru/cloud/mdb/internal/juggler/http"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	pgmeta "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthswagger "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	grpcmlockclient "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/grpc"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type AutoDuty struct {
	log   log.Logger
	cmsdb cmsdb.Client

	maxConcurrentTasks int

	moveWorkflowFactory        func() workflow.Workflow
	whipPrimaryWorkflowFactory func() workflow.Workflow
}

func (ad *AutoDuty) IsReady(ctx context.Context) error {
	return ad.cmsdb.IsReady(ctx)
}

func NewAutoDuty(
	log log.Logger,
	cmsdb cmsdb.Client,
	locker lockcluster.Locker,
	jglr juggler.API,
	client tasksclient.Client,
	db metadb.MetaDB,
	health healthapi.MDBHealthClient,
	shipmentsClient shipments.ShipmentClient,
	cfg config.Config,
	fqdnConverter fqdnlib.Converter,
) *AutoDuty {
	moveWorkflowFactory := func() workflow.Workflow {
		return workflow.NewMoveInstanceWorkflow(
			cmsdb,
			log,
			jglr,
			locker,
			client,
			db,
			health,
			shipmentsClient,
			cfg,
			fqdnConverter,
		)
	}
	whipPrimaryWorkflowFactory := func() workflow.Workflow {
		return workflow.NewWhipPrimaryWorkflow(
			cmsdb,
			log,
			locker,
			db,
			health,
			shipmentsClient,
			cfg,
		)
	}

	return &AutoDuty{
		log:                        log,
		cmsdb:                      cmsdb,
		maxConcurrentTasks:         cfg.MaxConcurrentTasks,
		moveWorkflowFactory:        moveWorkflowFactory,
		whipPrimaryWorkflowFactory: whipPrimaryWorkflowFactory,
	}
}

func NewAutoDutyFromConfig(ctx context.Context, logger log.Logger, cfg config.Config) *AutoDuty {
	db, err := pg.New(cfg.AutoDuty.Cmsdb, log.With(logger, log.String("cluster", cfg.AutoDuty.Cmsdb.DB)))
	if err != nil {
		logger.Fatal("could not create cmsdb client", log.Error(err))
	}

	if err = ready.WaitWithTimeout(ctx, 10*time.Second, db, &ready.DefaultErrorTester{Name: "cms database", L: logger}, time.Second); err != nil {
		logger.Fatal("cmsdb is not ready", log.Error(err))
	}

	metadbLogger := log.With(logger, log.String("cluster", cfg.AutoDuty.Metadb.DB))
	metadbCluster, err := pgutil.NewCluster(
		cfg.AutoDuty.Metadb,
		sqlutil.WithTracer(tracers.Log(metadbLogger)),
	)
	if err != nil {
		logger.Fatal("could not create metadb cluster", log.Error(err))
	}

	mdb := pgmeta.NewWithCluster(metadbCluster, log.With(metadbLogger, log.String("client", "common")))
	internalMdb := internalpgmeta.NewWithCluster(metadbCluster, log.With(metadbLogger, log.String("client", "internal")))

	otherLegsD := metadbdiscovery.NewMetaDBBasedDiscovery(mdb)

	if err = ready.WaitWithTimeout(ctx, 10*time.Second, internalMdb, &ready.DefaultErrorTester{Name: "metadb", L: logger}, time.Second); err != nil {
		logger.Fatal("metadb is not ready", log.Error(err))
	}

	taskClient := tasksclientprovider.NewTasksClient(internalMdb, mdb)

	client, err := grpc.NewTokenServiceClient(
		ctx,
		cfg.App.Environment.Services.Iam.V1.TokenService.Endpoint,
		cfg.App.AppName,
		grpcutil.DefaultClientConfig(),
		&grpcutil.PerRPCCredentialsStatic{},
		logger,
	)
	if err != nil {
		logger.Fatal("failed to initialize token service client", log.Error(err))
	}

	creds := client.ServiceAccountCredentials(iam.ServiceAccount{
		ID:    cfg.App.ServiceAccount.ID,
		KeyID: cfg.App.ServiceAccount.KeyID.Unmask(),
		Token: []byte(cfg.App.ServiceAccount.PrivateKey.Unmask()),
	})

	mlockclient, err := grpcmlockclient.NewFromConfig(ctx, cfg.Mlock, cfg.App.AppName, creds, logger)
	if err != nil {
		logger.Fatal("failed to initialize mlock client", log.Error(err))
	}

	locker := mlock.NewLocker(mlockclient, otherLegsD)

	jglr, err := jugglerclient.NewClient(cfg.Juggler, logger)
	if err != nil {
		logger.Fatal("could not create juggler client", log.Error(err))
	}

	health, err := healthswagger.NewClientTLSFromConfig(cfg.Health, logger)
	if err != nil {
		logger.Fatal("could not create health client", log.Error(err))
	}

	var deploy deployapiint.Client
	if cfg.Deploy.Token.Unmask() != "" {
		// OAuth creds
		deploy, err = deployapi.NewFromConfig(cfg.Deploy, logger)
	} else {
		// IAM creds
		deploy, err = deployapi.New(cfg.Deploy.URI, "", creds, cfg.Deploy.Transport.TLS, cfg.Deploy.Transport.Logging, logger)
	}

	if err != nil {
		logger.Fatal("could not create deploy client", log.Error(err))
	}

	shipmentClient := shipmentsprovider.NewShipmentProvider(deploy, mdb)

	fqdnConverter := fqdn.NewConverter(
		cfg.FQDNSuffixes.Controlplane, cfg.FQDNSuffixes.UnmanagedDataplane, cfg.FQDNSuffixes.ManagedDataplane)

	return NewAutoDuty(logger, db, locker, jglr, taskClient, mdb, health, shipmentClient, cfg, fqdnConverter)
}
