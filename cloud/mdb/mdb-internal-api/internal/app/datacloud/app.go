package datacloud

import (
	"net"
	"os"

	grpc_prometheus "github.com/grpc-ecosystem/go-grpc-prometheus"
	"github.com/jonboulle/clockwork"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/reflection"

	mdbv2 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v2"
	chconsv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/console/v1"
	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	consolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	kfconsv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/console/v1"
	kafkav1inner "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/inner/v1"
	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	grpcas "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	rmgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	vpcnop "a.yandex-team.ru/cloud/mdb/internal/compute/vpc/nop"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/fs"
	"a.yandex-team.ru/cloud/mdb/internal/fs/fsnotify"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/network"
	networkaws "a.yandex-team.ru/cloud/mdb/internal/network/doublecloud"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	s3http "a.yandex-team.ru/cloud/mdb/internal/s3/http"
	healthclient "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthswagger "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	chapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/clickhouse"
	chapicons "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/clickhouse/console"
	consoleapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/console"
	kafkaapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/kafka"
	kfapicons "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/kafka/console"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	commonapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	appconfig "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth/accessservice"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto/nacl"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	chprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider"
	commonprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	consoleprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/factory"
	kafkaprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	chlogsdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	pgmdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/pg"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/pkg/pillarsecrets"
	psclient "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/pkg/pillarsecrets/grpc"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

const userAgent = "Datacloud Internal API"

// Config describes base config
type Config struct {
	App                 app.Config                          `json:"app" yaml:"app"`
	API                 api.Config                          `json:"api" yaml:"api"`
	GRPC                grpcutil.ServeConfig                `json:"grpc" yaml:"grpc"`
	MetaDB              appconfig.MetaDBConfig              `json:"metadb" yaml:"metadb"`
	LogsDB              appconfig.LogsDBConfig              `json:"logsdb" yaml:"logsdb"`
	S3                  s3http.Config                       `json:"s3" yaml:"s3"`
	Health              healthswagger.Config                `json:"health" yaml:"health"`
	AccessService       appconfig.AccessServiceConfig       `json:"access_service" yaml:"access_service"`
	ServiceAccount      app.ServiceAccountConfig            `json:"service_account" yaml:"service_account"`
	TokenService        appconfig.TokenServiceConfig        `json:"token_service" yaml:"token_service"`
	ResourceManager     appconfig.ResourceManagerConfig     `json:"resource_manager" yaml:"resource_manager"`
	SLBCloseFile        appconfig.SLBCloseFileConfig        `json:"slb_close_file" yaml:"slb_close_file"`
	ReadOnlyFile        appconfig.ReadOnlyFileConfig        `json:"read_only_file" yaml:"read_only_file"`
	Logic               logic.Config                        `json:"logic" yaml:"logic"`
	Crypto              appconfig.CryptoConfig              `json:"crypto" yaml:"crypto"`
	PillarSecretsClient appconfig.PillarSecretServiceConfig `json:"pillar_secrets" yaml:"pillar_secrets"`
	VpcAPI              appconfig.VpcAPIConfig              `json:"vpc_api" yaml:"vpc_api"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultSLBCloseFileConfig() appconfig.SLBCloseFileConfig {
	return appconfig.SLBCloseFileConfig{
		FilePath: "/tmp/.datacloud-internal-api-close",
	}
}

func DefaultReadOnlyFileConfig() appconfig.ReadOnlyFileConfig {
	return appconfig.ReadOnlyFileConfig{
		FilePath: "/tmp/.datacloud-internal-api-read-only",
	}
}

// DefaultConfig returns default base config
func DefaultConfig() Config {
	cfg := Config{
		App:                 app.DefaultConfig(),
		API:                 api.DefaultConfig(),
		GRPC:                appconfig.DefaultGRPCConfig(),
		MetaDB:              appconfig.DefaultMetaDBConfig(),
		LogsDB:              appconfig.DefaultLogsDBConfig(),
		S3:                  s3http.DefaultConfig(),
		Health:              appconfig.DefaultHealthConfig(),
		AccessService:       appconfig.DefaultAccessServiceConfig(),
		TokenService:        appconfig.DefaultTokenServiceConfig(),
		ResourceManager:     appconfig.DefaultResourceManagerConfig(),
		SLBCloseFile:        DefaultSLBCloseFileConfig(),
		ReadOnlyFile:        DefaultReadOnlyFileConfig(),
		Logic:               logic.DefaultConfig(),
		Crypto:              appconfig.DefaultCryptoConfig(),
		PillarSecretsClient: appconfig.DefaultPillarSecretServiceConfig(),
		VpcAPI:              appconfig.DefaultVpcAPIConfig(),
	}

	cfg.App.Logging.File = "/var/log/datacloud-internal-api/api.log"
	cfg.App.Tracing.ServiceName = "datacloud-internal-api"
	cfg.API.Domain = api.DomainConfig{
		Public:  "yadc.io",
		Private: "private.yadc.io",
	}

	return cfg
}

type AppComponents struct {
	AccessService             as.AccessService
	TokenService              iam.TokenService
	ResourceManager           resmanager.Client
	MetaDB                    metadb.Backend
	S3Client                  s3.Client
	LogsDB                    logsdb.Backend
	SLBCloseFile              fs.FileWatcher
	ReadOnlyFile              fs.FileWatcher
	ClusterIDGenerator        generator.IDGenerator
	PillarIDGenerator         generator.IDGenerator
	SubClusterIDGenerator     generator.IDGenerator
	ShardIDGenerator          generator.IDGenerator
	TaskIDGenerator           generator.IDGenerator
	EventIDGenerator          generator.IDGenerator
	BackupIDGenerator         generator.IDGenerator
	PlatformHostnameGenerator generator.PlatformHostnameGenerator
	PillarSecretsClient       pillarsecrets.PillarSecretsClient
}

type App struct {
	*app.App
	AppComponents

	Config       Config
	grpcServer   *grpc.Server
	grpcListener net.Listener
}

const (
	ConfigName                 = "datacloud-internal-api.yaml"
	DefaultResourcesConfigName = "console_default_resources.yaml"
)

const (
	EnvMetaDBPassword   = "METADB_PASSWORD"
	EnvCryptoPrivateKey = "CRYPTO_PRIVATE_KEY"
	EnvS3SecretKey      = "S3_SECRET_KEY"
)

func Run(components AppComponents) {
	cfg := DefaultConfig()
	baseApp, err := app.New(app.DefaultServiceOptions(&cfg, ConfigName)...)
	if err != nil {
		panic(err)
	}

	if !cfg.MetaDB.DB.Password.FromEnv(EnvMetaDBPassword) {
		baseApp.L().Infof("%s is empty", EnvMetaDBPassword)
	}
	if !cfg.Crypto.PrivateKey.FromEnv(EnvCryptoPrivateKey) {
		baseApp.L().Infof("%s is empty", EnvCryptoPrivateKey)
	}
	if !cfg.S3.SecretKey.FromEnv(EnvS3SecretKey) {
		baseApp.L().Infof("%s is empty", EnvS3SecretKey)
	}

	err = config.Load(DefaultResourcesConfigName, &cfg.Logic.DefaultResources)
	if err != nil {
		baseApp.L().Errorf("failed to load default resources: %+v", err)
	} else {
		baseApp.L().Info("Successfully loaded default resources")
	}
	a, err := newApp(baseApp, cfg, components)
	if err != nil {
		baseApp.L().Errorf("failed to create app: %+v", err)
		os.Exit(1)
	}

	a.WaitForStop()
}

func newApp(baseApp *app.App, cfg Config, components AppComponents) (*App, error) {
	vctx := valid.NewValidationCtx()
	if verr := valid.Struct(vctx, cfg); verr != nil {
		return nil, xerrors.Errorf("invalid config: %w", verr)
	}

	// Initialize access service client
	if components.AccessService == nil {
		accessService, err := grpcas.NewClient(baseApp.ShutdownContext(), cfg.AccessService.Addr, userAgent, cfg.AccessService.ClientConfig, baseApp.L())
		if accessService == nil && err != nil {
			return nil, xerrors.Errorf("access service client init: %w", err)
		}
		components.AccessService = accessService
	}

	// Initialize token service client
	if components.TokenService == nil {
		tsClient, err := iamgrpc.NewTokenServiceClient(
			baseApp.ShutdownContext(),
			cfg.TokenService.Addr, userAgent,
			cfg.TokenService.ClientConfig,
			&grpcutil.PerRPCCredentialsStatic{},
			baseApp.L(),
		)
		if err != nil {
			return nil, xerrors.Errorf("token service client init: %w", err)
		}

		components.TokenService = tsClient
	}

	// Initialize resource manager client
	if components.ResourceManager == nil {
		var creds credentials.PerRPCCredentials
		if cfg.ServiceAccount.ID == "" {
			creds = &grpcutil.PerRPCCredentialsStatic{}
		} else {
			if !cfg.ServiceAccount.PrivateKey.FromEnv("SERVICE_ACCOUNT_PRIVATE_KEY") {
				baseApp.L().Info("SERVICE_ACCOUNT_PRIVATE_KEY is empty")
			}
			creds = components.TokenService.ServiceAccountCredentials(iam.ServiceAccount{
				ID:    cfg.ServiceAccount.ID,
				KeyID: cfg.ServiceAccount.KeyID.Unmask(),
				Token: []byte(cfg.ServiceAccount.PrivateKey.Unmask()),
			})
		}

		resMan, err := rmgrpc.NewClient(
			baseApp.ShutdownContext(),
			cfg.ResourceManager.Addr,
			userAgent,
			cfg.ResourceManager.ClientConfig,
			creds,
			baseApp.L(),
		)
		if err != nil {
			return nil, xerrors.Errorf("resource manager init: %w", err)
		}

		components.ResourceManager = resMan
	}

	// Initialize metadb client
	if components.MetaDB == nil {
		mdb, err := pgmdb.New(cfg.MetaDB.DB, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("metadb backend init: %w", err)
		}

		components.MetaDB = mdb
	}

	if components.S3Client == nil {
		client, err := s3http.New(cfg.S3, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("S3 client init: %w", err)
		}

		components.S3Client = client
	}

	// Initialize logsdb client
	if components.LogsDB == nil && !cfg.LogsDB.Disabled {
		if err := cfg.LogsDB.Config.DB.RegisterTLSConfig(); err != nil {
			return nil, xerrors.Errorf("logsdb clickhouse tls config registration: %w", err)
		}

		ldb, err := chlogsdb.New(cfg.LogsDB.Config, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("logs db backend init: %w", err)
		}

		components.LogsDB = ldb
	}

	// Initialize slb close watcher
	if components.SLBCloseFile == nil {
		slbc, err := fsnotify.NewFileWatcher(baseApp.ShutdownContext(), cfg.SLBCloseFile.FilePath, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("slb close file watcher init: %w", err)
		}

		components.SLBCloseFile = slbc
	}

	// Initialize read only watcher
	if components.ReadOnlyFile == nil {
		ro, err := fsnotify.NewFileWatcher(baseApp.ShutdownContext(), cfg.ReadOnlyFile.FilePath, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("read-only file watcher init: %w", err)
		}

		components.ReadOnlyFile = ro
	}

	// Initialize cluster id generator
	if components.ClusterIDGenerator == nil {
		components.ClusterIDGenerator = generator.NewClusterIDGenerator(cfg.API.CloudIDPrefix)
	}

	// Initialize subcluster id generator
	if components.SubClusterIDGenerator == nil {
		components.SubClusterIDGenerator = generator.NewSubClusterIDGenerator(cfg.API.CloudIDPrefix)
	}

	// Initialize shard id generator
	if components.ShardIDGenerator == nil {
		components.ShardIDGenerator = generator.NewShardIDGenerator(cfg.API.CloudIDPrefix)
	}

	// Initialize pillar id generator
	if components.PillarIDGenerator == nil {
		components.PillarIDGenerator = generator.NewPillarIDGenerator(cfg.API.CloudIDPrefix)
	}

	// Initialize task id generator
	if components.TaskIDGenerator == nil {
		components.TaskIDGenerator = generator.NewTaskIDGenerator(cfg.API.CloudIDPrefix)
	}

	// Initialize event id generator
	if components.EventIDGenerator == nil {
		components.EventIDGenerator = &generator.EventIDGenerator{}
	}

	// Initialize backup id generator
	if components.BackupIDGenerator == nil {
		components.BackupIDGenerator = &generator.BackupIDGenerator{}
	}

	// Initialize hostname generator
	if components.PlatformHostnameGenerator == nil {
		components.PlatformHostnameGenerator = generator.NewPlatformHostnameGenerator()
	}

	// Initialize pillar secrets client
	if components.PillarSecretsClient == nil {
		//baseApp.ShutdownContext(), cfg.AccessService.Addr, userAgent, cfg.AccessService.ClientConfig, baseApp.L()
		client, err := psclient.New(
			baseApp.ShutdownContext(),
			cfg.PillarSecretsClient.Addr,
			userAgent,
			cfg.PillarSecretsClient.ClientConfig,
			baseApp.L())
		if err != nil {
			return nil, err
		}

		components.PillarSecretsClient = client
	}

	// Initialize cluster servers generator
	clusterSecrets, err := factory.NewClusterSecretsGenerator()
	if err != nil {
		return nil, err
	}

	// Initialize health API client
	healthClient, err := healthswagger.NewClientTLSFromConfig(cfg.Health, baseApp.L())
	if err != nil {
		return nil, xerrors.Errorf("health client init: %w", err)
	}

	vpcClient := &vpcnop.Client{}
	baseApp.L().Info("Using vpc nop client")

	networkClient, err := networkaws.NewClient(baseApp.ShutdownContext(), cfg.VpcAPI.Address, userAgent, cfg.VpcAPI.DefaultIpv4CidrBlock, cfg.VpcAPI.ClientConfig, baseApp.L())
	if err != nil {
		return nil, err
	}

	cryptoClient, err := nacl.New(cfg.Crypto.PeersPublicKey, cfg.Crypto.PrivateKey.Unmask())
	if err != nil {
		return nil, xerrors.Errorf("crypto provider init: %w", err)
	}

	// Initialize application
	return NewApp(
		baseApp,
		cfg,
		components,
		healthClient,
		vpcClient,
		networkClient,
		cryptoClient,
		clusterSecrets,
	)
}

func NewApp(
	baseApp *app.App,
	cfg Config,
	components AppComponents,
	healthClient healthclient.MDBHealthClient,
	vpcClient vpc.Client,
	networkClient network.Client,
	cryptoClient crypto.Crypto,
	clusterSecretGenerator factory.ClusterSecrets,
) (*App, error) {
	// Revalidate config just in case (not all code-paths go through initial validation higher up)
	vctx := valid.NewValidationCtx()
	if verr := valid.Struct(vctx, cfg); verr != nil {
		return nil, xerrors.Errorf("invalid config: %w", verr)
	}

	a := &App{
		App:           baseApp,
		AppComponents: components,
		Config:        cfg,
	}

	clock := clockwork.NewRealClock()
	// Initialize auth checker
	authenticator := accessservice.NewAuthenticator(a.MetaDB, a.AccessService, a.App.L())

	// Initialize sessions
	sessions := factory.NewSessions(a.MetaDB, authenticator, a.ResourceManager, cfg.Logic.DefaultCloudQuota, cfg.Logic.ResourceModel)
	// Initialize cluster health
	clusterHealth := factory.NewHealth(healthClient, a.L())
	// Initialize backups
	backups := factory.NewBackups(a.MetaDB, a.S3Client, a.BackupIDGenerator, cfg.Logic)
	// Initialize clusters
	clusters := factory.NewClusters(a.MetaDB, a.ClusterIDGenerator, a.SubClusterIDGenerator, a.ShardIDGenerator, a.PillarIDGenerator, factory.NewDataCloudHostnameGenerator(), clusterSecretGenerator, sessions, clusterHealth, cfg.Logic, a.L())
	// Initialize events
	events := factory.NewEvents(a.MetaDB, a.EventIDGenerator)
	// Initialize search
	search := factory.NewSearch(a.MetaDB)
	// Initialize tasks
	tasks := factory.NewTasks(a.MetaDB, search, a.TaskIDGenerator, cfg.Logic, a.L())
	// Initialize compute
	compute := factory.NewCompute(vpcClient, networkClient, authenticator, nil, a.ResourceManager, nil)
	// Initialize API health
	apiHealth := commonprov.NewHealth(a.MetaDB, a.LogsDB, a.SLBCloseFile)
	// Initialize logs
	logs := commonprov.NewLogs(cfg.Logic, sessions, clusters, a.LogsDB, clock)
	// Initialize operations
	operations := commonprov.NewOperations(sessions, a.MetaDB)
	// Initialize quotas
	quotas := commonprov.NewQuotas(a.MetaDB, sessions, a.L())
	// Initialize ClickHouse service
	var chService *chprov.ClickHouse
	{
		chTasks := tasks
		if a.Config.Logic.ClickHouse.TasksPrefix != "" {
			chTasks = factory.NewTasks(a.MetaDB, search, generator.NewTaskIDGenerator(a.Config.Logic.ClickHouse.TasksPrefix), cfg.Logic, a.L())
		}

		chClusters := clusters
		if a.Config.Logic.ClickHouse.ClustersPrefix != "" {
			gen := generator.NewClusterIDGenerator(a.Config.Logic.ClickHouse.ClustersPrefix)
			chClusters = factory.NewClusters(a.MetaDB, gen, a.SubClusterIDGenerator, a.ShardIDGenerator, a.PillarIDGenerator, factory.NewDataCloudHostnameGenerator(), clusterSecretGenerator, sessions, clusterHealth, cfg.Logic, a.L())
		}

		chOperator := factory.NewOperator(sessions, a.MetaDB, chClusters, chClusters, chClusters, chClusters, cfg.Logic.ResourceModel, &a.Config.Logic, a.L())
		chService = chprov.New(a.Config.Logic, chOperator, backups, events, search, chTasks, compute, cryptoClient, components.PillarSecretsClient, a.App.L())
	}
	// Initialize Kafka service
	kafkaTasks := tasks
	if a.Config.Logic.Kafka.TasksPrefix != "" {
		kafkaTasks = factory.NewTasks(a.MetaDB, search, generator.NewTaskIDGenerator(a.Config.Logic.Kafka.TasksPrefix), cfg.Logic, a.L())
	}
	kfClusters := clusters
	if a.Config.Logic.Kafka.ClustersPrefix != "" {
		clusterIDGen := generator.NewClusterIDGenerator(a.Config.Logic.Kafka.ClustersPrefix)
		kfClusters = factory.NewClusters(a.MetaDB, clusterIDGen, a.SubClusterIDGenerator, a.ShardIDGenerator, a.PillarIDGenerator, factory.NewDataCloudHostnameGenerator(), clusterSecretGenerator, sessions, clusterHealth, cfg.Logic, a.L())
	}
	kfOperator := factory.NewOperator(sessions, a.MetaDB, kfClusters, kfClusters, kfClusters, kfClusters, cfg.Logic.ResourceModel, &a.Config.Logic, a.L())
	kafkaService := kafkaprov.New(a.Config.Logic, cfg.API.Domain, kfOperator, backups, events, search, kafkaTasks, cryptoClient, compute, components.PillarSecretsClient, a.L())

	// Initialize console
	clusterSpecific := map[clustermodels.Type]console.ClusterSpecific{
		clustermodels.TypeClickHouse: chService,
		clustermodels.TypeKafka:      kafkaService,
	}
	consoleService := consoleprov.New(a.Config.Logic, authenticator, sessions, components.MetaDB, clusterHealth, networkClient, clusterSpecific)
	// Initialize gRPC service
	backoff := retry.New(cfg.API.Retry)
	readOnlyChecker := interceptors.NewReadOnlyChecker(
		components.ReadOnlyFile,
		api.DataCloudPrefix,
		api.DatacloudReadOnlyMethods,
	)
	grpc_prometheus.EnableHandlingTimeHistogram()
	options := append(
		[]grpc.ServerOption{
			grpc.UnaryInterceptor(
				interceptors.ChainUnaryServerInterceptors(
					backoff,
					cfg.API.ExposeErrorDebug,
					readOnlyChecker,
					cfg.GRPC.Logging,
					a.L(),
					api.NewUnaryServerInterceptorAuth(api.IsDataCloudMethod),
				),
			),
			grpc.StreamInterceptor(
				interceptors.ChainStreamServerInterceptors(
					cfg.API.ExposeErrorDebug,
					readOnlyChecker,
					a.L(),
					api.NewStreamServerInterceptorAuth(api.IsDataCloudMethod),
				),
			),
		},
		grpcutil.DefaultKeepaliveServerOptions()...,
	)

	// Create server
	server := grpc.NewServer(options...)

	// Init Health services
	hs := commonapi.NewHealthService(apiHealth)
	grpchealth.RegisterHealthServer(server, hs)

	// Init services common for other database services
	clusterService := &grpcapi.ClusterService{Logs: logs, L: a.App.L(), Config: cfg.API}

	// Init ClickHouse services
	chcs := chapi.NewClusterService(clusterService, chService)
	chv1.RegisterClusterServiceServer(server, chcs)
	chos := chapi.NewOperationService(operations, a.App.L())
	chv1.RegisterOperationServiceServer(server, chos)
	chver := chapi.NewVersionService(cfg.Logic.ClickHouse)
	chv1.RegisterVersionServiceServer(server, chver)
	chbackups := chapi.NewBackupService(clusterService, chService)
	chv1.RegisterBackupServiceServer(server, chbackups)
	chConsoleCloud := chapicons.NewCloudService(consoleService)
	chconsv1.RegisterCloudServiceServer(server, chConsoleCloud)
	chConsoleCluster := chapicons.NewClusterService(cfg.Logic.ClickHouse, consoleService, chService)
	chconsv1.RegisterClusterServiceServer(server, chConsoleCluster)

	// Init Kafka services
	kafkacs := kafkaapi.NewClusterService(clusterService, kafkaService, operations)
	kfv1.RegisterClusterServiceServer(server, kafkacs)
	kfos := kafkaapi.NewOperationService(operations, a.App.L())
	kfv1.RegisterOperationServiceServer(server, kfos)
	kafkaver := kafkaapi.NewVersionService()
	kfv1.RegisterVersionServiceServer(server, kafkaver)
	kfConsoleCloud := kfapicons.NewCloudService(consoleService)
	kfconsv1.RegisterCloudServiceServer(server, kfConsoleCloud)
	kfConsoleCluster := kfapicons.NewClusterService(cfg.Logic.Kafka, consoleService, kafkaService)
	kfconsv1.RegisterClusterServiceServer(server, kfConsoleCluster)
	kafkats := &kafkaapi.TopicService{Kafka: kafkaService, L: a.App.L()}
	kfv1.RegisterTopicServiceServer(server, kafkats)
	kafkaits := &kafkaapi.InnerTopicService{Kafka: kafkaService}
	kafkav1inner.RegisterTopicServiceServer(server, kafkaits)

	// Init console services
	consoleCluster := consoleapi.NewClusterService(consoleService)
	consolev1.RegisterClusterServiceServer(server, consoleCluster)

	// Init Quota Service
	qts := commonapi.NewQuotaService(quotas, a.App.L())
	mdbv2.RegisterQuotaServiceServer(server, qts)

	// Register server for reflection
	reflection.Register(server)
	// Register server for metrics
	grpc_prometheus.Register(server)

	a.grpcServer = server

	listener, err := grpcutil.Serve(a.grpcServer, a.Config.GRPC.Addr, a.App.L())
	if err != nil {
		return nil, err
	}

	a.grpcServer = server
	a.grpcListener = listener
	return a, nil
}

func (a *App) WaitForStop() {
	a.App.WaitForStop()
	if err := grpcutil.Shutdown(a.grpcServer, a.Config.GRPC.ShutdownTimeout); err != nil {
		a.L().Error("failed to shutdown gRPC server", log.Error(err))
	}
}

func (a *App) GRPCPort() int {
	return a.grpcListener.Addr().(*net.TCPAddr).Port
}
