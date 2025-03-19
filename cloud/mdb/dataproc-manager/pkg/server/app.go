package server

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/pflag"

	cc "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/compute-service"
	iam "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/iam-token-service"
	ig "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/instance-group-service"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/datastore/redis"
	internal "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api/http"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	computeiam "a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sentry/raven"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// App main application object - handles setup and teardown
type App struct {
	logger log.Logger

	cfg appConfig
}

type appConfig struct {
	LogLevel                   log.Level                     `json:"loglevel" yaml:"loglevel"`
	ServerCfg                  Config                        `json:"server" yaml:"server"`
	RedisCfg                   redis.Config                  `json:"redis" yaml:"redis"`
	InternalAPICfg             internal.Config               `json:"internal-api" yaml:"internal-api"`
	Sentry                     raven.Config                  `json:"sentry" yaml:"sentry"`
	ComputeServiceConfig       cc.ComputeServiceConfig       `json:"compute-service" yaml:"compute-service"`
	InstanceGroupServiceConfig ig.InstanceGroupServiceConfig `json:"instance-group-service" yaml:"instance-group-service"`
	IamTokenServiceConfig      iam.IamTokenServiceConfig     `json:"iam-token-service" yaml:"iam-token-service"`
	ServiceAccount             app.ServiceAccountConfig      `json:"service_account" yaml:"service_account"`
}

type ServiceAccountConfig struct {
	ID         string        `json:"id" yaml:"id"`
	KeyID      secret.String `json:"key_id" yaml:"key_id"`
	PrivateKey secret.String `json:"private_key" yaml:"private_key"`
}

func defaultAppConfig() appConfig {
	return appConfig{
		LogLevel:                   log.DebugLevel,
		ServerCfg:                  DefaultConfig(),
		RedisCfg:                   redis.DefaultConfig(),
		InternalAPICfg:             internal.DefaultConfig(),
		ComputeServiceConfig:       cc.DefaultConfig(),
		InstanceGroupServiceConfig: ig.DefaultConfig(),
		IamTokenServiceConfig:      iam.DefaultConfig(),
	}
}

const (
	configName      = "dataproc-manager.yaml"
	flagKeyLogLevel = "dataproc-manager-loglevel"
)

var appFlags *pflag.FlagSet

func init() {
	defcfg := defaultAppConfig()
	appFlags = pflag.NewFlagSet("App", pflag.ExitOnError)
	appFlags.String(flagKeyLogLevel, defcfg.LogLevel.String(), "Set log level")
	pflag.CommandLine.AddFlagSet(appFlags)
	flags.RegisterConfigPathFlagGlobal()
}

// New constructs application object
// nolint: gocyclo
func New() *App {
	logger, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		fmt.Printf("failed to initialize logger: %s\n", err)
		os.Exit(1)
	}

	cfg := defaultAppConfig()
	if err = config.Load(configName, &cfg); err != nil {
		logger.Errorf("failed to load application config, using defaults: %s", err)
		cfg = defaultAppConfig()
	}

	if !cfg.ServiceAccount.FromEnv("SA") {
		logger.Warn("SA_PRIVATE_KEY is not set in env")
	}
	if !cfg.RedisCfg.Password.FromEnv("REDIS_PASSWORD") {
		logger.Warn("REDIS_PASSWORD is empty")
	}
	if !cfg.InternalAPICfg.AccessID.FromEnv("INTERNAL_API_ACCESS_ID") {
		logger.Warn("INTERNAL_API_ACCESS_ID is empty")
	}
	if !cfg.InternalAPICfg.AccessSecret.FromEnv("INTERNAL_API_ACCESS_SECRET") {
		logger.Warn("INTERNAL_API_ACCESS_SECRET is empty")
	}
	if !cfg.Sentry.DSN.FromEnv("SENTRY_DSN") {
		logger.Warn("SENTRY_DSN is empty")
	}

	if appFlags.Changed(flagKeyLogLevel) {
		var ll string
		if ll, err = appFlags.GetString(flagKeyLogLevel); err == nil {
			cfg.LogLevel, err = log.ParseLevel(ll)
			if err != nil {
				logger.Fatalf("failed to parse loglevel: %s", err)
			}
		}
	}

	logger.Infof("Setting log level to %s", cfg.LogLevel)
	// Recreate logger with specified loglevel
	oldLogger := logger
	logger, err = zap.New(zap.KVConfig(cfg.LogLevel))
	if err != nil {
		oldLogger.Fatalf("failed to initialize logger: %s", err)
	}

	logger.Debugf("Using config: %+v", cfg)

	return &App{
		logger: logger,
		cfg:    cfg,
	}
}

// Run main app
func (app *App) Run() {
	datastore := redis.New(app.logger, app.cfg.RedisCfg)
	internalAPI, err := internal.New(app.cfg.InternalAPICfg, app.logger)
	if err != nil {
		panic(fmt.Sprintf("failed to initialize intapi client: %s", err))
	}

	tokenServiceClient, err := iamgrpc.NewTokenServiceClient(
		context.Background(),
		app.cfg.IamTokenServiceConfig.ServiceURL,
		"Dataproc-Manager",
		grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{
					CAFile: app.cfg.IamTokenServiceConfig.CAPath,
				},
			},
		},
		&grpcutil.PerRPCCredentialsStatic{},
		app.logger,
	)
	if err != nil {
		app.logger.Fatalf("Failed to initialize token service client: %v", err)
	}

	sa := app.cfg.ServiceAccount
	creds := tokenServiceClient.ServiceAccountCredentials(computeiam.ServiceAccount{
		ID:    sa.ID,
		KeyID: sa.KeyID.Unmask(),
		Token: []byte(sa.PrivateKey.Unmask()),
	})

	iamTokenServiceClient := iam.New(app.cfg.IamTokenServiceConfig, creds, app.logger)
	computeServiceClient := cc.New(app.cfg.ComputeServiceConfig, creds, app.logger)
	instanceGroupServiceClient := ig.New(app.cfg.InstanceGroupServiceConfig, app.logger, iamTokenServiceClient)

	// Initialize sentry
	if err = raven.Init(app.cfg.Sentry); err != nil {
		app.logger.Errorf("sentry is not initialized: %s", err.Error())
	}

	serverRunner, err := NewServer(
		app.cfg.ServerCfg,
		datastore,
		internalAPI,
		app.logger,
		computeServiceClient,
		instanceGroupServiceClient,
	)
	if err != nil {
		app.logger.Fatalf("Failed to serve: %v", err)
	}
	serverRunner.Run()
}
