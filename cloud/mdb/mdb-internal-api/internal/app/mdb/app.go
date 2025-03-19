package mdb

import (
	"net"
	"os"

	grpc_prometheus "github.com/grpc-ecosystem/go-grpc-prometheus"
	"github.com/jonboulle/clockwork"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/reflection"

	airflowv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/airflow/v1"
	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	chv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1/console"
	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	esv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1/console"
	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	gpv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1/console"
	kafkav1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	kafkav1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1/console"
	kafkav1inner "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1/inner"
	mongov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1"
	mysqlv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mysql/v1"
	mysqlv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mysql/v1/console"
	osv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/opensearch/v1"
	osv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/opensearch/v1/console"
	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	pgv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1/console"
	redisv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/redis/v1"
	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	ssv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1/console"
	mdbv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1"
	consolev1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/console"
	supportv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/support"
	mdbv2 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v2"
	metastorev1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/metastore/v1"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	grpcas "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	computeapi "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	computegrpc "a.yandex-team.ru/cloud/mdb/internal/compute/compute/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	iamnop "a.yandex-team.ru/cloud/mdb/internal/compute/iam/nop"
	"a.yandex-team.ru/cloud/mdb/internal/compute/marketplace"
	httpmarketplace "a.yandex-team.ru/cloud/mdb/internal/compute/marketplace/http"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	rmgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	vpcgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/vpc/grpc"
	vpcnop "a.yandex-team.ru/cloud/mdb/internal/compute/vpc/nop"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/fs"
	"a.yandex-team.ru/cloud/mdb/internal/fs/fsnotify"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/network"
	metanetwork "a.yandex-team.ru/cloud/mdb/internal/network/meta"
	portonetwork "a.yandex-team.ru/cloud/mdb/internal/network/porto"
	"a.yandex-team.ru/cloud/mdb/internal/racktables"
	racktableshttp "a.yandex-team.ru/cloud/mdb/internal/racktables/http"
	racktablesnop "a.yandex-team.ru/cloud/mdb/internal/racktables/nop"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	s3http "a.yandex-team.ru/cloud/mdb/internal/s3/http"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	healthclient "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthswagger "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	airflowapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/airflow"
	chapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/clickhouse"
	commonapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	consoleapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/console"
	esapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/elasticsearch"
	gpapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/greenplum"
	kafkaapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/kafka"
	metastoreapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/metastore"
	mongodbapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/mongodb"
	mysqlapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/mysql"
	osapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/opensearch"
	pgapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/postgresql"
	redisapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/redis"
	ssapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/sqlserver"
	supportapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/support"
	appconfig "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth/accessservice"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto/nacl"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	airflowprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/provider"
	chprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider"
	commonprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	consoleprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console/provider"
	esprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/factory"
	gpprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/provider"
	kafkaprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider"
	metastoreprovider "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/provider"
	mongoperfdiagdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/perfdiagdb"
	mongoperfdiagdbimpl "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/perfdiagdb/clickhouse"
	mongodbprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/provider"
	myperfdiagdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/perfdiagdb"
	myperfdiagdbimpl "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/perfdiagdb/clickhouse"
	mysqlprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/provider"
	osprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/provider"
	pgperfdiagdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/perfdiagdb"
	pgperfdiagdbimpl "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/perfdiagdb/clickhouse"
	pgprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/provider"
	redisprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/provider"
	ssprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/provider"
	supportprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/support/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	chlogsdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	pgmdb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/pg"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

const userAgent = "MDB Internal API"

// Config describes base config
type Config struct {
	App                   app.Config                      `json:"app" yaml:"app"`
	API                   api.Config                      `json:"api" yaml:"api"`
	GRPC                  grpcutil.ServeConfig            `json:"grpc" yaml:"grpc"`
	MetaDB                appconfig.MetaDBConfig          `json:"metadb" yaml:"metadb"`
	LogsDB                appconfig.LogsDBConfig          `json:"logsdb" yaml:"logsdb"`
	S3                    s3http.Config                   `json:"s3" yaml:"s3"`
	S3SecureBackups       s3http.Config                   `json:"s3_secure_backups" yaml:"s3_secure_backups"`
	Health                healthswagger.Config            `json:"health" yaml:"health"`
	PerfDiagDB            PerfDiagDBConfig                `json:"perfdiagdb" yaml:"perfdiagdb"`
	MongoDBPerfDiagDB     PerfDiagDBConfig                `json:"perfdiagdb_mongodb" yaml:"perfdiagdb_mongodb"`
	AccessService         appconfig.AccessServiceConfig   `json:"access_service" yaml:"access_service"`
	LicenseService        appconfig.LicenseServiceConfig  `json:"license_service" yaml:"license_service"`
	TokenService          appconfig.TokenServiceConfig    `json:"token_service" yaml:"token_service"`
	ServiceAccount        app.ServiceAccountConfig        `json:"service_account" yaml:"service_account"`
	ResourceManager       appconfig.ResourceManagerConfig `json:"resource_manager" yaml:"resource_manager"`
	VPC                   VPCConfig                       `json:"vpc" yaml:"vpc"`
	SLBCloseFile          appconfig.SLBCloseFileConfig    `json:"slb_close_file" yaml:"slb_close_file"`
	ReadOnlyFile          appconfig.ReadOnlyFileConfig    `json:"read_only_file" yaml:"read_only_file"`
	Logic                 logic.Config                    `json:"logic" yaml:"logic"`
	Crypto                appconfig.CryptoConfig          `json:"crypto" yaml:"crypto"`
	Compute               ComputeConfig                   `json:"compute" yaml:"compute"`
	YandexTeamIntegration YandexTeamIntegrationConfig     `json:"yandex_team_integration" yaml:"yandex_team_integration"`
	Racktables            racktableshttp.Config           `json:"racktables" yaml:"racktables"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

type PerfDiagDBConfig struct {
	Disabled bool          `json:"disabled" yaml:"disabled"`
	DB       chutil.Config `json:"db" yaml:"db"`
}

func DefaultPerfDiagDBConfig() PerfDiagDBConfig {
	return PerfDiagDBConfig{
		DB: pgperfdiagdbimpl.DefaultConfig(),
	}
}

func DefaultMongoDBPerfDiagDBConfig() PerfDiagDBConfig {
	return PerfDiagDBConfig{
		DB:       mongoperfdiagdbimpl.DefaultConfig(),
		Disabled: true,
	}
}

func DefaultSLBCloseFileConfig() appconfig.SLBCloseFileConfig {
	return appconfig.SLBCloseFileConfig{
		FilePath: "/tmp/.mdb-internal-api-close",
	}
}

func DefaultReadOnlyFileConfig() appconfig.ReadOnlyFileConfig {
	return appconfig.ReadOnlyFileConfig{
		FilePath: "/tmp/.mdb-internal-api-read-only",
	}
}

type VPCConfig struct {
	URI          string                `json:"uri" yaml:"uri"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultVPCConfig() VPCConfig {
	return VPCConfig{
		ClientConfig: appconfig.DefaultGRPCClientConfigWithAllCAs(),
	}
}

type ComputeConfig struct {
	URI          string                `json:"uri" yaml:"uri"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultComputeConfig() ComputeConfig {
	return ComputeConfig{
		ClientConfig: appconfig.DefaultGRPCClientConfigWithAllCAs(),
	}
}

type YandexTeamIntegrationConfig struct {
	URI          string                `json:"uri" yaml:"uri"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

func DefaultYandexTeamIntegrationConfig() YandexTeamIntegrationConfig {
	return YandexTeamIntegrationConfig{
		ClientConfig: appconfig.DefaultGRPCClientConfigWithAllCAs(),
	}
}

// DefaultConfig returns default base config
func DefaultConfig() Config {
	cfg := Config{
		App:                   app.DefaultConfig(),
		API:                   api.DefaultConfig(),
		GRPC:                  appconfig.DefaultGRPCConfig(),
		MetaDB:                appconfig.DefaultMetaDBConfig(),
		LogsDB:                appconfig.DefaultLogsDBConfig(),
		S3:                    s3http.DefaultConfig(),
		S3SecureBackups:       s3http.DefaultConfig(),
		Health:                appconfig.DefaultHealthConfig(),
		PerfDiagDB:            DefaultPerfDiagDBConfig(),
		MongoDBPerfDiagDB:     DefaultMongoDBPerfDiagDBConfig(),
		AccessService:         appconfig.DefaultAccessServiceConfig(),
		LicenseService:        appconfig.DefaultLicenseServiceConfig(),
		TokenService:          appconfig.DefaultTokenServiceConfig(),
		ResourceManager:       appconfig.DefaultResourceManagerConfig(),
		VPC:                   DefaultVPCConfig(),
		SLBCloseFile:          DefaultSLBCloseFileConfig(),
		ReadOnlyFile:          DefaultReadOnlyFileConfig(),
		Logic:                 logic.DefaultConfig(),
		Crypto:                appconfig.DefaultCryptoConfig(),
		Compute:               DefaultComputeConfig(),
		YandexTeamIntegration: DefaultYandexTeamIntegrationConfig(),
		Racktables:            racktableshttp.DefaultConfig(),
	}

	cfg.App.Logging.File = "/var/log/mdb-internal-api/api.log"
	cfg.App.Tracing.ServiceName = "mdb-internal-api"
	return cfg
}

type AppComponents struct {
	AccessService                     as.AccessService
	TokenService                      iam.TokenService
	ResourceManager                   resmanager.Client
	MetaDB                            metadb.Backend
	S3Client                          s3.Client
	S3SecureBackupsClient             s3.Client
	LogsDB                            logsdb.Backend
	PgPerfDiagDB                      pgperfdiagdb.Backend
	MyPerfDiagDB                      myperfdiagdb.Backend
	MongoPerfDiagDB                   mongoperfdiagdb.Backend
	SLBCloseFile                      fs.FileWatcher
	ReadOnlyFile                      fs.FileWatcher
	ClusterIDGenerator                generator.IDGenerator
	PillarIDGenerator                 generator.IDGenerator
	SubClusterIDGenerator             generator.IDGenerator
	ShardIDGenerator                  generator.IDGenerator
	TaskIDGenerator                   generator.IDGenerator
	EventIDGenerator                  generator.IDGenerator
	PlatformHostnameGenerator         generator.PlatformHostnameGenerator
	BackupIDGenerator                 generator.IDGenerator
	ElasticsearchExtensionIDGenerator generator.IDGenerator
}

type App struct {
	*app.App
	AppComponents

	Config       Config
	grpcServer   *grpc.Server
	grpcListener net.Listener
}

const (
	ConfigName                 = "mdb-internal-api.yaml"
	DefaultResourcesConfigName = "console_default_resources.yaml"
)

const (
	EnvMetaDBPassword            = "METADB_PASSWORD"
	EnvLogsDBPassword            = "LOGSDB_PASSWORD"
	EnvPerfDiagDBPassword        = "PERFDIAGDB_PASSWORD"
	EnvPerfDiagDBMongoDBPassword = "PERFDIAGDB_MONGODB_PASSWORD"
	EnvS3AccessKey               = "S3_ACCESS_KEY"
	EnvS3SecretKey               = "S3_SECRET_KEY"
	EnvS3SecureBackupsAccessKey  = "S3_SECURE_BACKUPS_ACCESS_KEY"
	EnvS3SecureBackupsSecretKey  = "S3_SECURE_BACKUPS_SECRET_KEY"
	EnvInternalAPISA             = "INTERNAL_API_SA"
	EnvCryptoPrivateKey          = "CRYPTO_PRIVATE_KEY"
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
	if !cfg.LogsDB.DB.Password.FromEnv(EnvLogsDBPassword) {
		baseApp.L().Infof("%s is empty", EnvLogsDBPassword)
	}
	if !cfg.PerfDiagDB.DB.Password.FromEnv(EnvPerfDiagDBPassword) {
		baseApp.L().Infof("%s is empty", EnvPerfDiagDBPassword)
	}
	if !cfg.MongoDBPerfDiagDB.DB.Password.FromEnv(EnvPerfDiagDBMongoDBPassword) {
		baseApp.L().Infof("%s is empty", EnvPerfDiagDBMongoDBPassword)
	}
	if !cfg.S3.AccessKey.FromEnv(EnvS3AccessKey) {
		baseApp.L().Infof("%s is empty", EnvS3AccessKey)
	}
	if !cfg.S3.SecretKey.FromEnv(EnvS3SecretKey) {
		baseApp.L().Infof("%s is empty", EnvS3SecretKey)
	}
	if !cfg.S3SecureBackups.AccessKey.FromEnv(EnvS3SecureBackupsAccessKey) {
		baseApp.L().Infof("%s is empty", EnvS3SecureBackupsAccessKey)
	}
	if !cfg.S3SecureBackups.SecretKey.FromEnv(EnvS3SecureBackupsSecretKey) {
		baseApp.L().Infof("%s is empty", EnvS3SecureBackupsSecretKey)
	}
	if !cfg.ServiceAccount.FromEnv(EnvInternalAPISA) {
		baseApp.L().Infof("%s_PRIVATE_KEY is empty", EnvInternalAPISA)
	}
	if !cfg.Crypto.PrivateKey.FromEnv(EnvCryptoPrivateKey) {
		baseApp.L().Infof("%s is empty", EnvCryptoPrivateKey)
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
		return nil, xerrors.Errorf("failed to validate config: %w", verr)
	}

	// Initialize access service client
	if components.AccessService == nil {
		asClient, err := grpcas.NewClient(baseApp.ShutdownContext(), cfg.AccessService.Addr, userAgent, cfg.AccessService.ClientConfig, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize access service client: %w", err)
		}

		components.AccessService = asClient
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
			return nil, xerrors.Errorf("failed to initialize token service client: %w", err)
		}

		components.TokenService = tsClient
	}

	iamServiceAccount := iam.ServiceAccount{
		ID:    cfg.ServiceAccount.ID,
		KeyID: cfg.ServiceAccount.KeyID.Unmask(),
		Token: []byte(cfg.ServiceAccount.PrivateKey.Unmask()),
	}

	// Initialize license client
	var licenseClient marketplace.LicenseService
	if cfg.LicenseService.Addr != "" {
		if components.TokenService == nil {
			return nil, xerrors.New("unable to construct license client, cause token-service not initialized")
		}
		var err error
		licenseClient, err = httpmarketplace.NewClient(
			baseApp.ShutdownContext(),
			cfg.LicenseService.Addr,
			baseApp.L(),
			components.TokenService,
			iamServiceAccount,
			cfg.LicenseService.HTTPConfig,
		)
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize license client: %w", err)
		}
	}

	// Initialize resource manager client
	if components.ResourceManager == nil {
		var creds credentials.PerRPCCredentials
		if cfg.ServiceAccount.ID == "" {
			creds = &grpcutil.PerRPCCredentialsStatic{}
		} else {
			creds = components.TokenService.ServiceAccountCredentials(iamServiceAccount)
		}

		c, err := rmgrpc.NewClient(
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

		components.ResourceManager = c
	}

	// Initialize metadb client
	if components.MetaDB == nil {
		mdb, err := pgmdb.New(cfg.MetaDB.DB, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize metadb backend: %w", err)
		}

		components.MetaDB = mdb
	}

	if components.S3Client == nil {
		client, err := s3http.New(cfg.S3, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize S3 client: %w", err)
		}

		components.S3Client = client
	}

	if components.S3SecureBackupsClient == nil {
		client, err := s3http.New(cfg.S3SecureBackups, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize S3 client: %w", err)
		}

		components.S3SecureBackupsClient = client
	}

	// Initialize logsdb client
	if components.LogsDB == nil && !cfg.LogsDB.Disabled {
		if err := cfg.LogsDB.Config.DB.RegisterTLSConfig(); err != nil {
			return nil, xerrors.Errorf("failed to register logsdb clickhouse tls config: %w", err)
		}

		ldb, err := chlogsdb.New(cfg.LogsDB.Config, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize logs db backend: %w", err)
		}

		components.LogsDB = ldb
	}

	// Initialize perfdiag client
	if (components.PgPerfDiagDB == nil || components.MyPerfDiagDB == nil) && !cfg.PerfDiagDB.Disabled {
		if err := cfg.PerfDiagDB.DB.Validate(); err != nil {
			return nil, err
		}

		if err := cfg.PerfDiagDB.DB.RegisterTLSConfig(); err != nil {
			return nil, xerrors.Errorf("failed to register perfdiag clickhouse tls config: %w", err)
		}

		var err error
		if components.PgPerfDiagDB == nil {
			components.PgPerfDiagDB, err = pgperfdiagdbimpl.New(cfg.PerfDiagDB.DB, baseApp.L())
			if err != nil {
				return nil, xerrors.Errorf("failed to initialize postgresql perfdiag db backend: %w", err)
			}
		}
		if components.MyPerfDiagDB == nil {
			components.MyPerfDiagDB, err = myperfdiagdbimpl.New(cfg.PerfDiagDB.DB, baseApp.L())
			if err != nil {
				return nil, xerrors.Errorf("failed to initialize mysql perfdiag db backend: %w", err)
			}
		}
	}

	// Initialize perfdiag client
	if (components.MongoPerfDiagDB == nil) && !cfg.MongoDBPerfDiagDB.Disabled {
		if err := cfg.MongoDBPerfDiagDB.DB.Validate(); err != nil {
			return nil, err
		}

		if err := cfg.MongoDBPerfDiagDB.DB.RegisterTLSConfig(); err != nil {
			return nil, xerrors.Errorf("failed to register perfdiag clickhouse tls config: %w", err)
		}

		var err error
		if components.MongoPerfDiagDB == nil {
			components.MongoPerfDiagDB, err = mongoperfdiagdbimpl.New(cfg.MongoDBPerfDiagDB.DB, baseApp.L())
			if err != nil {
				return nil, xerrors.Errorf("failed to initialize mongodb perfdiag db backend: %w", err)
			}
		}
	}

	// Initialize slb close watcher
	if components.SLBCloseFile == nil {
		slbc, err := fsnotify.NewFileWatcher(baseApp.ShutdownContext(), cfg.SLBCloseFile.FilePath, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize slb close file watcher: %w", err)
		}

		components.SLBCloseFile = slbc
	}

	// Initialize read only watcher
	if components.ReadOnlyFile == nil {
		ro, err := fsnotify.NewFileWatcher(baseApp.ShutdownContext(), cfg.ReadOnlyFile.FilePath, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize read only file watcher: %w", err)
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

	// Initialize hostname generator
	if components.PlatformHostnameGenerator == nil {
		components.PlatformHostnameGenerator = generator.NewPlatformHostnameGenerator()
	}

	// Initialize backup id generator
	if components.BackupIDGenerator == nil {
		components.BackupIDGenerator = &generator.BackupIDGenerator{}
	}

	// Initialize elasticsearch extension id generator
	if components.ElasticsearchExtensionIDGenerator == nil {
		components.ElasticsearchExtensionIDGenerator = generator.NewRandomIDGenerator()
	}

	// Initialize cluster servers generator
	clusterSecrets, err := factory.NewClusterSecretsGenerator()
	if err != nil {
		return nil, err
	}

	// Initialize health API client
	healthClient, err := healthswagger.NewClientTLSFromConfig(cfg.Health, baseApp.L())
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize Health client: %w", err)
	}

	// Initialize vpc API client
	var vpcClient vpc.Client
	if cfg.VPC.URI != "" {
		if components.TokenService == nil {
			return nil, xerrors.New("unable to construct VPC client, cause token-service not initialized")
		}
		vpcCreds := components.TokenService.ServiceAccountCredentials(iamServiceAccount)
		vpcClient, err = vpcgrpc.NewClient(baseApp.ShutdownContext(), cfg.VPC.URI, userAgent, cfg.VPC.ClientConfig, vpcCreds, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize VPC client: %w", err)
		}
	} else {
		vpcClient = &vpcnop.Client{}
		baseApp.L().Info("vpc uri is not specified. Using nop client")
	}

	// Initialize racktables API client
	var racktablesClient racktables.Client
	if cfg.Racktables.Endpoint != "" {
		if cfg.Racktables.Token.Unmask() == "" {
			return nil, xerrors.New("unable to construct Racktables client, cause token not specified")
		}
		racktablesClient, err = racktableshttp.NewClient(cfg.Racktables, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("unable to initialize Racktables client: %w", err)
		}
	} else {
		racktablesClient = &racktablesnop.Client{}
		baseApp.L().Info("racktables endpoint is not specified. Using nop client")
	}

	// Initialize Yandex Team Integration API client
	var yandexTeamIntegrationClient iam.AbcService
	if cfg.YandexTeamIntegration.URI != "" {
		if components.TokenService == nil {
			return nil, xerrors.New("unable to construct Yandex-Team Integration client, cause token-service not initialized")
		}
		ytiCreds := components.TokenService.ServiceAccountCredentials(iamServiceAccount)
		yandexTeamIntegrationClient, err = iamgrpc.NewAbcServiceClient(baseApp.ShutdownContext(), cfg.YandexTeamIntegration.URI, userAgent, cfg.YandexTeamIntegration.ClientConfig, ytiCreds, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("unable to initialize Yandex Team Integration client: %w", err)
		}
	} else {
		yandexTeamIntegrationClient = &iamnop.AbcServiceClient{}
		baseApp.L().Info("IAM Yandex-Team Integration uri is not specified. Using nop client")
	}

	// Initialize Porto Network Provider client
	portoNetworkClient := portonetwork.NewClient(racktablesClient, yandexTeamIntegrationClient, baseApp.L())

	// Initialize Meta Network Provider client
	networkClient, err := metanetwork.NewClient(string(cfg.Logic.EnvironmentVType), vpcClient, portoNetworkClient, nil, baseApp.L())
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize network provider: %w", err)
	}

	cryptoClient, err := nacl.New(cfg.Crypto.PeersPublicKey, cfg.Crypto.PrivateKey.Unmask())
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize crypto provider: %w", err)
	}

	var hostGroupClient computeapi.HostGroupService
	if cfg.Compute.URI != "" {
		if components.TokenService == nil {
			return nil, xerrors.New("unable to construct host groups client because token-service is not initialized")
		}
		computeCreds := components.TokenService.ServiceAccountCredentials(iamServiceAccount)
		hostGroupClient, err = computegrpc.NewHostGroupServiceClient(baseApp.ShutdownContext(), cfg.Compute.URI, userAgent, cfg.Compute.ClientConfig, computeCreds, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize host groups client: %w", err)
		}
	}

	var hostTypeClient computeapi.HostTypeService
	if cfg.Compute.URI != "" {
		if components.TokenService == nil {
			return nil, xerrors.New("unable to construct host groups client because token-service is not initialized")
		}
		computeCreds := components.TokenService.ServiceAccountCredentials(iamServiceAccount)
		hostTypeClient, err = computegrpc.NewHostTypeServiceClient(baseApp.ShutdownContext(), cfg.Compute.URI, userAgent, cfg.Compute.ClientConfig, computeCreds, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize host type client: %w", err)
		}
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
		hostGroupClient,
		hostTypeClient,
		licenseClient,
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
	hostGroupClient computeapi.HostGroupService,
	hostTypeClient computeapi.HostTypeService,
	licenseClient marketplace.LicenseService,
) (*App, error) {
	// Revalidate config just in case (not all code-paths go through initial validation higher up)
	vctx := valid.NewValidationCtx()
	if verr := valid.Struct(vctx, cfg); verr != nil {
		return nil, xerrors.Errorf("failed to validate config: %w", verr)
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
	secureBackups := factory.NewBackups(a.MetaDB, a.S3SecureBackupsClient, a.BackupIDGenerator, cfg.Logic)
	// Initialize clusters
	clusters := factory.NewClusters(a.MetaDB, a.ClusterIDGenerator, a.SubClusterIDGenerator, a.ShardIDGenerator, a.PillarIDGenerator, factory.NewMdbHostnameGenerator(a.PlatformHostnameGenerator), clusterSecretGenerator, sessions, clusterHealth, cfg.Logic, a.L())
	// Initialize operator
	operator := factory.NewOperator(sessions, a.MetaDB, clusters, clusters, clusters, clusters, cfg.Logic.ResourceModel, &a.Config.Logic, a.L())
	// Initialize events
	events := factory.NewEvents(a.MetaDB, a.EventIDGenerator)
	// Initialize search
	search := factory.NewSearch(a.MetaDB)
	// Initialize tasks
	tasks := factory.NewTasks(a.MetaDB, search, a.TaskIDGenerator, cfg.Logic, a.L())
	// Initialize compute
	compute := factory.NewCompute(vpcClient, networkClient, authenticator, hostGroupClient, a.ResourceManager, hostTypeClient)
	// Initialize API health
	apiHealth := commonprov.NewHealth(a.MetaDB, a.LogsDB, a.SLBCloseFile)
	// Initialize logs
	logs := commonprov.NewLogs(cfg.Logic, sessions, clusters, a.LogsDB, clock)
	// Initialize operations
	operations := commonprov.NewOperations(sessions, a.MetaDB)
	// Initialize quotas
	quotas := commonprov.NewQuotas(a.MetaDB, sessions, a.L())
	// Init ClickHouse service
	chService := chprov.New(a.Config.Logic, operator, backups, events, search, tasks, compute, cryptoClient, nil, a.App.L())
	// Initialize console
	clusterSpecific := map[clustermodels.Type]console.ClusterSpecific{
		clustermodels.TypeClickHouse: chService,
	}
	consoleService := consoleprov.New(a.Config.Logic, authenticator, sessions, components.MetaDB, clusterHealth, networkClient, clusterSpecific)
	// Initialize support logic
	support := supportprov.NewSupport(a.MetaDB, clusterHealth, sessions, a.App.L())

	// Initialize perf diag services
	pgPerfDiag := pgprov.NewPerfDiag(cfg.Logic, sessions, a.PgPerfDiagDB, clock)
	mysqlPerfDiag := mysqlprov.NewPerfDiag(cfg.Logic, sessions, a.MyPerfDiagDB, clock)
	mongoPerfDiag := mongodbprov.NewPerfDiag(cfg.Logic, sessions, a.MongoPerfDiagDB, clock)

	// Initialize gRPC service
	backoff := retry.New(cfg.API.Retry)
	readOnlyChecker := interceptors.NewReadOnlyChecker(
		components.ReadOnlyFile,
		api.YandexCloudPrefix,
		api.YandexCloudReadOnlyMethods,
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
					api.NewUnaryServerInterceptorAuth(api.IsYandexCloudMethod),
					interceptors.NewUnaryServerInterceptorRestricted(cfg.API.RestrictedChecker),
				),
			),
			grpc.StreamInterceptor(
				interceptors.ChainStreamServerInterceptors(
					cfg.API.ExposeErrorDebug,
					readOnlyChecker,
					a.L(),
					api.NewStreamServerInterceptorAuth(api.IsYandexCloudMethod),
					interceptors.NewStreamServerInterceptorRestricted(cfg.API.RestrictedChecker),
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
	// Init PostgreSQL services
	pgService := pgprov.New(a.Config.Logic, a.App.L(), operator, backups, events, tasks, compute, consoleService)
	pgcs := pgapi.NewClusterService(clusterService)
	pgv1.RegisterClusterServiceServer(server, pgcs)
	pgds := pgapi.NewDatabaseService(pgService, a.App.L())
	pgv1.RegisterDatabaseServiceServer(server, pgds)
	pgpd := pgapi.NewPerformanceDiagnosticsService(pgPerfDiag, a.App.L())
	pgv1.RegisterPerformanceDiagnosticsServiceServer(server, pgpd)

	// Init ClickHouse services
	chcs := chapi.NewClusterService(clusterService, chService, operations, cfg.Logic.SaltEnvs)
	chv1.RegisterClusterServiceServer(server, chcs)
	chos := chapi.NewOperationService(operations, chService, cfg.Logic.SaltEnvs, a.App.L())
	chv1.RegisterOperationServiceServer(server, chos)
	chbs := chapi.NewBackupService(chService, a.App.L())
	chv1.RegisterBackupServiceServer(server, chbs)
	chus := chapi.NewUserService(chService, a.App.L())
	chv1.RegisterUserServiceServer(server, chus)
	chds := chapi.NewDatabaseService(chService, a.App.L())
	chv1.RegisterDatabaseServiceServer(server, chds)
	chmls := chapi.NewMLModelService(chService, a.App.L())
	chv1.RegisterMlModelServiceServer(server, chmls)
	chfs := chapi.NewFormatSchemaService(chService, a.App.L())
	chv1.RegisterFormatSchemaServiceServer(server, chfs)
	chrps := chapi.NewResourcePresetService(chService, consoleService, a.App.L())
	chv1.RegisterResourcePresetServiceServer(server, chrps)
	chvs := chapi.NewVersionsService(chService, a.App.L())
	chv1.RegisterVersionsServiceServer(server, chvs)

	// Init MongoDB services
	mgService := mongodbprov.New(a.Config.Logic, operator, backups, events, tasks, cryptoClient)
	mgcs := mongodbapi.NewClusterService(clusterService, mgService, cfg.Logic.SaltEnvs)
	mongov1.RegisterClusterServiceServer(server, mgcs)
	mgds := mongodbapi.NewDatabaseService(mgService, a.App.L())
	mongov1.RegisterDatabaseServiceServer(server, mgds)
	mgbs := mongodbapi.NewBackupService(mgService, a.App.L())
	mongov1.RegisterBackupServiceServer(server, mgbs)
	mongopd := mongodbapi.NewPerformanceDiagnosticsService(mongoPerfDiag, a.App.L())
	mongov1.RegisterPerformanceDiagnosticsServiceServer(server, mongopd)

	// Init MySQL services
	mysqlService := mysqlprov.New(a.Config.Logic, a.App.L(), operator, backups, events, tasks, compute, consoleService)
	mysqlcs := mysqlapi.NewClusterService(clusterService, mysqlService)
	mysqlv1.RegisterClusterServiceServer(server, mysqlcs)
	mysqlpd := mysqlapi.NewPerformanceDiagnosticsService(mysqlPerfDiag, a.App.L())
	mysqlv1.RegisterPerformanceDiagnosticsServiceServer(server, mysqlpd)

	// Init Redis services
	redisService := redisprov.New(operator, backups, events, tasks, compute)
	rediscs := redisapi.NewClusterService(clusterService, redisService)
	redisv1.RegisterClusterServiceServer(server, rediscs)
	redisbs := redisapi.NewBackupService(redisService, a.App.L())
	redisv1.RegisterBackupServiceServer(server, redisbs)

	// Init Kafka services
	kafkaTasks := tasks
	if a.Config.Logic.Kafka.TasksPrefix != "" {
		kafkaTasks = factory.NewTasks(a.MetaDB, search, generator.NewTaskIDGenerator(a.Config.Logic.Kafka.TasksPrefix), cfg.Logic, a.L())
	}
	kafkaOperator := operator
	if a.Config.Logic.Kafka.ClustersPrefix != "" {
		clusterIDGen := generator.NewClusterIDGenerator(a.Config.Logic.Kafka.ClustersPrefix)
		kafkaClusters := factory.NewClusters(a.MetaDB, clusterIDGen, a.SubClusterIDGenerator, a.ShardIDGenerator,
			a.PillarIDGenerator, factory.NewMdbHostnameGenerator(a.PlatformHostnameGenerator), clusterSecretGenerator,
			sessions, clusterHealth, cfg.Logic, a.L())
		kafkaOperator = factory.NewOperator(sessions, a.MetaDB, kafkaClusters, kafkaClusters, kafkaClusters,
			kafkaClusters, cfg.Logic.ResourceModel, &a.Config.Logic, a.L())
	}
	kafkaService := kafkaprov.New(a.Config.Logic, cfg.API.Domain, kafkaOperator, backups, events, search, kafkaTasks, cryptoClient, compute, nil, a.L())
	kafkacs := kafkaapi.NewClusterService(clusterService, kafkaService, operations, a.Config.Logic.SaltEnvs)
	kafkav1.RegisterClusterServiceServer(server, kafkacs)
	kafkaos := kafkaapi.NewOperationService(operations, kafkaService, a.Config.Logic.SaltEnvs, a.App.L())
	kafkav1.RegisterOperationServiceServer(server, kafkaos)
	kafkats := &kafkaapi.TopicService{Kafka: kafkaService, L: a.App.L()}
	kafkav1.RegisterTopicServiceServer(server, kafkats)
	kafkaits := &kafkaapi.InnerTopicService{Kafka: kafkaService}
	kafkav1inner.RegisterTopicServiceServer(server, kafkaits)
	kafkaus := &kafkaapi.UserService{Kafka: kafkaService, L: a.App.L()}
	kafkav1.RegisterUserServiceServer(server, kafkaus)
	kafkacns := &kafkaapi.ConnectorService{Kafka: kafkaService, L: a.App.L()}
	kafkav1.RegisterConnectorServiceServer(server, kafkacns)
	kfrps := kafkaapi.NewResourcePresetService(kafkaService, consoleService, a.App.L())
	kafkav1.RegisterResourcePresetServiceServer(server, kfrps)

	// Init Metastore services
	metastoreService := metastoreprovider.New(a.Config.Logic, operator, backups, events, search, kafkaTasks, cryptoClient, compute, nil, a.L())
	metastoreClusterService := metastoreapi.NewClusterService(clusterService, metastoreService, operations)
	metastorev1.RegisterClusterServiceServer(server, metastoreClusterService)
	metastoreOperationService := metastoreapi.NewOperationService(operations, metastoreService, a.App.L())
	metastorev1.RegisterOperationServiceServer(server, metastoreOperationService)

	// Init SQLServer services
	ssTasks := tasks
	if a.Config.Logic.SQLServer.TasksPrefix != "" {
		ssTasks = factory.NewTasks(a.MetaDB, search, generator.NewTaskIDGenerator(a.Config.Logic.SQLServer.TasksPrefix), cfg.Logic, a.L())
	}
	ssService := ssprov.New(a.Config.Logic, a.App.L(), operator, backups, events, search, ssTasks, cryptoClient, compute, consoleService, licenseClient)
	sscs := ssapi.NewClusterService(clusterService, ssService, operations, cfg.Logic.SaltEnvs)
	ssv1.RegisterClusterServiceServer(server, sscs)
	ssds := &ssapi.DatabaseService{SQLServer: ssService, L: a.App.L()}
	ssv1.RegisterDatabaseServiceServer(server, ssds)
	ssus := &ssapi.UserService{SQLServer: ssService, L: a.App.L()}
	ssv1.RegisterUserServiceServer(server, ssus)
	ssbs := &ssapi.BackupService{SQLServer: ssService, L: a.App.L()}
	ssv1.RegisterBackupServiceServer(server, ssbs)
	ssos := ssapi.NewOperationService(operations, ssService, cfg.Logic.SaltEnvs, a.App.L())
	ssv1.RegisterOperationServiceServer(server, ssos)
	ssrps := ssapi.NewResourcePresetService(consoleService)
	ssv1.RegisterResourcePresetServiceServer(server, ssrps)

	// Init Geenplum services
	gpTasks := tasks
	if a.Config.Logic.Greenplum.TasksPrefix != "" {
		gpTasks = factory.NewTasks(a.MetaDB, search, generator.NewTaskIDGenerator(a.Config.Logic.Greenplum.TasksPrefix), cfg.Logic, a.L())
	}
	gpService := gpprov.New(a.Config.Logic, a.App.L(), operator, backups, events, search, gpTasks, cryptoClient, compute, consoleService)
	gpcs := gpapi.NewClusterService(clusterService, gpService, operations, cfg.Logic.SaltEnvs)
	gpv1.RegisterClusterServiceServer(server, gpcs)
	gpos := gpapi.NewOperationService(operations, gpService, cfg.Logic.SaltEnvs, a.App.L())
	gpv1.RegisterOperationServiceServer(server, gpos)
	gpbs := gpapi.NewBackupService(gpService, a.App.L())
	gpv1.RegisterBackupServiceServer(server, gpbs)
	gprp := gpapi.NewResourcePresetService(consoleService)
	gpv1.RegisterResourcePresetServiceServer(server, gprp)

	// Init ElasticSearch services
	esTasks := tasks
	if a.Config.Logic.Elasticsearch.TasksPrefix != "" {
		esTasks = factory.NewTasks(a.MetaDB, search, generator.NewTaskIDGenerator(a.Config.Logic.Elasticsearch.TasksPrefix), cfg.Logic, a.L())
	}
	esService := esprov.New(a.Config.Logic, authenticator, operator, secureBackups, events, search, esTasks, compute, cryptoClient, a.ElasticsearchExtensionIDGenerator, a.L())
	escs := esapi.NewClusterService(clusterService, esService, operations, cfg.Logic.SaltEnvs)
	esv1.RegisterClusterServiceServer(server, escs)
	esus := &esapi.UserService{Elasticsearch: esService, L: a.App.L()}
	esv1.RegisterUserServiceServer(server, esus)
	esas := esapi.NewAuthService(esService, a.App.L())
	esv1.RegisterAuthServiceServer(server, esas)
	esops := esapi.NewOperationService(operations, esService, cfg.Logic.SaltEnvs, a.App.L())
	esv1.RegisterOperationServiceServer(server, esops)
	esbs := esapi.NewBackupService(esService, a.App.L())
	esv1.RegisterBackupServiceServer(server, esbs)
	eses := esapi.NewExtensionService(esService, a.App.L())
	esv1.RegisterExtensionServiceServer(server, eses)
	esrps := esapi.NewResourcePresetService(esService, consoleService, a.App.L())
	esv1.RegisterResourcePresetServiceServer(server, esrps)

	// Init OpenSearch services
	osTasks := tasks
	if a.Config.Logic.Elasticsearch.TasksPrefix != "" {
		osTasks = factory.NewTasks(a.MetaDB, search, generator.NewTaskIDGenerator(a.Config.Logic.Elasticsearch.TasksPrefix), cfg.Logic, a.L())
	}
	osService := osprov.New(a.Config.Logic, authenticator, operator, secureBackups, events, search, osTasks, compute, cryptoClient, a.ElasticsearchExtensionIDGenerator, a.L())
	oscs := osapi.NewClusterService(clusterService, osService, operations, cfg.Logic.SaltEnvs)
	osv1.RegisterClusterServiceServer(server, oscs)
	osas := osapi.NewAuthService(osService, a.App.L())
	osv1.RegisterAuthServiceServer(server, osas)
	osops := osapi.NewOperationService(operations, osService, cfg.Logic.SaltEnvs, a.App.L())
	osv1.RegisterOperationServiceServer(server, osops)
	osbs := osapi.NewBackupService(osService, a.App.L())
	osv1.RegisterBackupServiceServer(server, osbs)
	oses := osapi.NewExtensionService(osService, a.App.L())
	osv1.RegisterExtensionServiceServer(server, oses)
	osrps := osapi.NewResourcePresetService(osService, consoleService, a.App.L())
	osv1.RegisterResourcePresetServiceServer(server, osrps)

	// Init Airflow services
	airflowService := airflowprov.New(a.Config.Logic, operator, backups, events, search, tasks, cryptoClient, compute, nil, a.L())
	airflowcs := airflowapi.NewClusterService(clusterService, airflowService, operations)
	airflowv1.RegisterClusterServiceServer(server, airflowcs)

	// Init console services
	consoleCUDService := &consoleapi.CUDService{Console: consoleService}
	consolev1.RegisterCUDServiceServer(server, consoleCUDService)
	consoleNetworkService := &consoleapi.NetworkService{Console: consoleService}
	consolev1.RegisterNetworkServiceServer(server, consoleNetworkService)
	consoleClusterService := &consoleapi.ClusterService{Console: consoleService, L: a.App.L()}
	consolev1.RegisterClusterServiceServer(server, consoleClusterService)

	chccs := chapi.NewConsoleClusterService(consoleService, chService, a.Config.Logic.SaltEnvs)
	chv1console.RegisterClusterServiceServer(server, chccs)

	kafkaccs := kafkaapi.NewConsoleClusterService(consoleService, kafkaService, a.Config.Logic.SaltEnvs)
	kafkav1console.RegisterClusterServiceServer(server, kafkaccs)

	mysqlccs := mysqlapi.NewConsoleClusterService(consoleService, mysqlService, a.Config.Logic.SaltEnvs)
	mysqlv1console.RegisterClusterServiceServer(server, mysqlccs)

	pgccs := pgapi.NewConsoleClusterService(consoleService, pgService, a.Config.Logic.SaltEnvs)
	pgv1console.RegisterClusterServiceServer(server, pgccs)

	ssccs := ssapi.NewConsoleClusterService(consoleService, ssService, a.Config.Logic.SaltEnvs)
	ssv1console.RegisterClusterServiceServer(server, ssccs)

	gpccs := gpapi.NewConsoleClusterService(consoleService, gpService, a.Config.Logic.SaltEnvs)
	gpv1console.RegisterClusterServiceServer(server, gpccs)

	esccs := esapi.NewConsoleClusterService(consoleService, esService, a.Config.Logic.SaltEnvs)
	esv1console.RegisterClusterServiceServer(server, esccs)

	osccs := osapi.NewConsoleClusterService(consoleService, osService, a.Config.Logic.SaltEnvs)
	osv1console.RegisterClusterServiceServer(server, osccs)

	// Init Operations services
	ops := &commonapi.OperationService{Operations: operations, Kafka: kafkaos, L: a.App.L()}
	mdbv1.RegisterOperationServiceServer(server, ops)

	// Init Quota Service
	qts := commonapi.NewQuotaService(quotas, a.App.L())
	mdbv2.RegisterQuotaServiceServer(server, qts)

	supportResolveClusterService := supportapi.NewClusterResolveService(support, a.App.L())
	supportv1.RegisterClusterResolveServiceServer(server, supportResolveClusterService)

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
