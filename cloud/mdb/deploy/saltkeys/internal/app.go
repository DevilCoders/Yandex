package internal

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys"
	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys/skcli"
	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/fs/fsnotify"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi/cherrypy"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi/configs"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// App main application object - handles setup and teardown
type App struct {
	logger log.Logger

	ctx    context.Context
	cfg    Config
	dapi   deployapi.Client
	skeys  saltkeys.Keys
	km     *KeysManager
	pinger *MinionPinger
}

type Config struct {
	SaltAPI        SaltAPIConfig         `json:"saltapi" yaml:"saltapi"`
	CAPath         string                `json:"capath" yaml:"capath"`
	DeployAPIURL   string                `json:"deployapiurl" yaml:"deployapiurl"`
	DeployAPIToken string                `json:"deployapitoken" yaml:"deployapitoken"`
	IAMAddr        string                `json:"addr" yaml:"addr"`
	IAMGRPCCC      grpcutil.ClientConfig `json:"config" yaml:"config"`
	IAMID          string                `json:"iam_id" yaml:"iam_id"`
	IAMKeyID       secret.String         `json:"iam_key_id" yaml:"iam_key_id"`
	IAMPrivateKey  secret.String         `json:"iam_private_key" yaml:"iam_private_key"`
	LogLevel       log.Level             `json:"loglevel" yaml:"loglevel"`
	LogHTTPBody    bool                  `json:"loghttpbody" yaml:"loghttpbody"`
	KeysManager    KeysManagerConfig     `json:"keys_manager" yaml:"keys_manager"`
	MinionPinger   MinionPingerConfig    `json:"minion_pinger" yaml:"minion_pinger"`
}

func DefaultConfig() Config {
	return Config{
		SaltAPI:      DefaultSaltAPIConfig(),
		LogLevel:     log.DebugLevel,
		KeysManager:  DefaultKeysManagerConfig(),
		MinionPinger: DefaultMinionPingerConfig(),
	}
}

type SaltAPIConfig struct {
	Cherrypy cherrypy.ClientConfig `json:"cherrypy" yaml:"cherrypy"`
	URL      string                `json:"url" yaml:"url"`
}

func DefaultSaltAPIConfig() SaltAPIConfig {
	return SaltAPIConfig{Cherrypy: cherrypy.DefaultClientConfig(), URL: "http://localhost:8000"}
}

const (
	appName         = "mdb-deploy-salt-keys"
	configName      = "mdb-deploy-saltkeys.yaml"
	flagKeyLogLevel = "mdb-loglevel"
)

var appFlags *pflag.FlagSet

func init() {
	defcfg := DefaultConfig()
	appFlags = pflag.NewFlagSet("App", pflag.ExitOnError)
	appFlags.String(flagKeyLogLevel, defcfg.LogLevel.String(), "Set log level")
	pflag.CommandLine.AddFlagSet(appFlags)
	flags.RegisterConfigPathFlagGlobal()
}

// NewApp constructs application object
// nolint: gocyclo
func NewApp() *App {
	logger, err := zap.New(zap.JSONConfig(log.DebugLevel))
	if err != nil {
		fmt.Printf("failed to initialize logger: %s\n", err)
		os.Exit(1)
	}

	cfg := DefaultConfig()
	if err = config.Load(configName, &cfg); err != nil {
		logger.Errorf("failed to load application config, using defaults: %s", err)
		cfg = DefaultConfig()
	}

	if !cfg.SaltAPI.Cherrypy.Auth.Password.FromEnv("DEPLOY_SALT_API_PASSWORD") {
		logger.Warn("salt api password is empty")
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
	logger, err = zap.New(zap.JSONConfig(cfg.LogLevel))
	if err != nil {
		oldLogger.Fatalf("failed to initialize logger: %s", err)
	}

	minionCfg, errs := configs.LoadMinion()
	if errs != nil {
		logger.Warnf("Loaded master's minion config with errors %v: %+v", errs, minionCfg)
	} else if minionCfg.ID != "" {
		logger.Infof("Loaded master's minion config without errors: %+v", minionCfg)
	}

	if cfg.KeysManager.SaltMasterFQDN == "" {
		logger.Info("Salt master name is not set in our config.")
		if minionCfg.ID != "" {
			cfg.KeysManager.SaltMasterFQDN = minionCfg.ID
			logger.Infof("Using salt master name from master minion's config: %s", cfg.KeysManager.SaltMasterFQDN)
		} else {
			if cfg.KeysManager.SaltMasterFQDN, err = os.Hostname(); err != nil {
				logger.Fatalf("failed to get master name from hostname, no fallbacks left, bailing out: %s", err)
			} else {
				logger.Infof("Using salt master name from hostname: %s", cfg.KeysManager.SaltMasterFQDN)
			}
		}
	}

	logger.Infof("Using config %+v", cfg)

	ctx := context.Background()
	var iamCred iam.CredentialsService
	if cfg.IAMID != "" {
		tokenService, err := iamc.NewTokenServiceClient(ctx, cfg.IAMAddr, appName, cfg.IAMGRPCCC, &grpcutil.PerRPCCredentialsStatic{}, logger)
		if err != nil {
			logger.Fatalf("failed to get token service client: %s", err)
		}
		iamCred = tokenService.ServiceAccountCredentials(iam.ServiceAccount{
			ID:    cfg.IAMID,
			KeyID: cfg.IAMKeyID.Unmask(),
			Token: []byte(cfg.IAMPrivateKey.Unmask()),
		})
	}

	dapi, err := restapi.New(
		cfg.DeployAPIURL,
		cfg.DeployAPIToken,
		iamCred,
		httputil.TLSConfig{CAFile: cfg.CAPath},
		httputil.LoggingConfig{LogRequestBody: cfg.LogHTTPBody, LogResponseBody: cfg.LogHTTPBody},
		logger,
	)
	if err != nil {
		logger.Fatalf("failed to initialize deploy api client: %s", err)
	}

	return &App{
		logger: logger,
		ctx:    ctx,
		cfg:    cfg,
		skeys:  skcli.New(logger),
		dapi:   dapi,
	}
}

// Run main app
func (app *App) Run() {
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		app.logger.Fatalf("Failed to initialize fsnotify watcher: %s", err)
	}
	defer func() {
		if err = watcher.Close(); err != nil {
			app.logger.Errorf("Failed to close watcher: %s", err)
		}
	}()

	app.logger.Debug("Created fsnotify watcher")

	ctx := signals.WithCancelOnSignal(app.ctx)

	if err = ready.Wait(ctx, app.dapi, &ready.DefaultErrorTester{Name: "deploy api", L: app.logger}, time.Second); err != nil {
		app.logger.Fatalf("Failed to wait for Deploy API to become ready: %s", err)
	}

	app.logger.Debug("Deploy API is ready")

	app.km, err = NewKeysManager(ctx, app.cfg.KeysManager, app.dapi, app.skeys, watcher, app.logger)
	if err != nil {
		app.logger.Fatalf("Failed to create keys manager: %s", err)
	}

	// Create salt api client
	saclient, err := cherrypy.New(
		app.cfg.SaltAPI.URL,
		app.cfg.SaltAPI.Cherrypy.HTTP.TLS,
		app.cfg.SaltAPI.Cherrypy.HTTP.Logging,
		app.logger,
	)
	if err != nil {
		app.logger.Fatalf("failed to initialize salt-api client: %s", err)
	}

	auth := saclient.NewAuth(app.cfg.SaltAPI.Cherrypy.Auth)
	go auth.AutoAuth(ctx, app.cfg.SaltAPI.Cherrypy.ReAuthPeriod, app.cfg.SaltAPI.Cherrypy.AuthAttemptTimeout, app.logger)

	app.pinger = NewMinionPinger(ctx, app.cfg.MinionPinger, saclient, auth, app.skeys, app.logger)

	for range ctx.Done() {
	}
}
