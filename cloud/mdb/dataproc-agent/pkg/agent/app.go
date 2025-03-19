package agent

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"os/exec"
	"strings"
	"sync/atomic"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/apiclient/grpcclient"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs"
	jobconfig "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs/config"
	metaparser "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/metaparser/http"
	proxyagent "a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/agent"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// App main application object - handles setup and teardown
type App struct {
	logger log.Logger
	cfg    appConfig
}

type appConfig struct {
	LogLevel            log.Level           `json:"loglevel" yaml:"loglevel"`
	AgentCfg            Config              `json:"agent" yaml:"agent"`
	MetricsServerConfig MetricsServerConfig `json:"metrics_server" yaml:"metrics_server"`
	JobsConfig          jobconfig.Config    `json:"jobs" yaml:"jobs"`
	GRPCConfig          grpcclient.Config   `json:"grpc" yaml:"grpc"`
	UIProxyConfig       proxyagent.Config   `json:"uiproxy" yaml:"uiproxy"`
	FakeMetaInfo        MetaInfo            `json:"fake_meta_info" yaml:"fake_meta_info"`
	MetaHTTPConfig      metaparser.Config   `json:"metaparser" yaml:"metaparser"`
	MetaTimeout         time.Duration       `json:"meta_timeout" yaml:"meta_timeout"`
}

func defaultAppConfig() appConfig {
	return appConfig{
		LogLevel:            log.DebugLevel,
		AgentCfg:            DefaultConfig(),
		MetricsServerConfig: MetricsServerDefaultConfig(),
		JobsConfig:          jobconfig.DefaultConfig(),
		GRPCConfig:          grpcclient.DefaultConfig(),
		MetaHTTPConfig:      metaparser.DefaultConfig(),
		MetaTimeout:         time.Minute,
	}
}

const (
	configName      = "dataproc-agent.yaml"
	flagKeyLogLevel = "dataproc-agent-loglevel"
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
	cfg.JobsConfig.Yarn.ServerAddress = cfg.AgentCfg.Health.YARNAPIURL
	cfg.JobsConfig.Cid = cfg.AgentCfg.Cid
	cfg.UIProxyConfig.AgentID = cfg.AgentCfg.Cid

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

type MetaInfo struct {
	TopologyRevision     int64  `yaml:"topology_revision"`
	S3Bucket             string `yaml:"s3_bucket"`
	ServiceAccountToken  string `yaml:"service_account_token"`
	FolderID             string `yaml:"folder_id"`
	UIProxyEnabled       bool   `yaml:"ui_proxy_enabled"`
	StaticNodesNumber    int64
	IsAutoscalingCluster bool
}

func (app *App) fetchMetadata(ctx context.Context) (MetaInfo, error) {
	metaGetter := metaparser.New(app.cfg.MetaHTTPConfig)
	ctx, cancel := context.WithTimeout(ctx, app.cfg.MetaTimeout)
	defer cancel()

	computeMetadata, err := metaGetter.GetComputeMetadata(ctx)
	if err != nil {
		return MetaInfo{}, err
	}
	app.cfg.JobsConfig.S3Logger.BucketName = computeMetadata.InstanceAttributes.UserData.S3Bucket
	app.cfg.AgentCfg.Services = computeMetadata.InstanceAttributes.UserData.Services
	var staticNodesNumber int64
	isAutoscalingCluster := false
	for _, subcluster := range computeMetadata.InstanceAttributes.UserData.Topology.Subclusters {
		isAutoscalingSubcluster := subcluster.InstanceGroupConfig.ScalePolicy.AutoScale.CustomRules != nil
		isAutoscalingCluster = isAutoscalingCluster || isAutoscalingSubcluster
		if subcluster.Role != "hadoop_cluster.masternode" && !isAutoscalingSubcluster {
			staticNodesNumber += subcluster.HostsCount
		}
	}

	return MetaInfo{
		TopologyRevision:     computeMetadata.InstanceAttributes.UserData.Topology.Revision,
		S3Bucket:             computeMetadata.InstanceAttributes.UserData.S3Bucket,
		ServiceAccountToken:  computeMetadata.IAMToken,
		FolderID:             computeMetadata.InstanceAttributes.FolderID,
		UIProxyEnabled:       computeMetadata.InstanceAttributes.UserData.UIProxy,
		StaticNodesNumber:    staticNodesNumber,
		IsAutoscalingCluster: isAutoscalingCluster,
	}, nil
}

// Run main app
func (app *App) Run() {
	ctx := context.Background()
	_, stopUIProxyAgent := context.WithCancel(ctx)
	initialized := false
	agentRunner := (*Agent)(nil)
	jobsRunner := (*jobs.Jobs)(nil)
	metricsServer := (*MetricsServer)(nil)

	token := atomic.Value{}
	getToken := func() string {
		return token.Load().(string)
	}

	folderID := atomic.Value{}
	getFolderID := func() string {
		return folderID.Load().(string)
	}

	meta := MetaInfo{}
	for newMeta := range app.metaStream() {
		metaChanged := newMeta != meta
		uiProxySwitched := newMeta.UIProxyEnabled != meta.UIProxyEnabled

		if metaChanged {
			meta = newMeta
			token.Store(meta.ServiceAccountToken)
			folderID.Store(meta.FolderID)
		}

		if !initialized || uiProxySwitched {
			app.logger.Infof("UI proxy module state will be initialized or changed")
			app.ensureKnoxState(meta.UIProxyEnabled)
			if meta.UIProxyEnabled {
				var uiProxyCtx context.Context
				uiProxyCtx, stopUIProxyAgent = context.WithCancel(ctx)
				err := app.runProxyAgent(uiProxyCtx, getToken)
				if err != nil {
					app.logger.Fatalf("Failed to start UI proxy agent: %s", err)
				} else {
					app.logger.Infof("UI proxy agent started successfully")
				}
			} else {
				stopUIProxyAgent()
				app.logger.Infof("UI proxy agent has been stopped")
			}
		}

		if !initialized {
			apiClient, err := grpcclient.New(
				app.cfg.GRPCConfig,
				getToken,
				getFolderID,
				app.cfg.AgentCfg.Cid,
				"dataproc-agent",
				app.logger)
			if err != nil {
				app.logger.Fatal(err.Error())
			}

			if !app.cfg.AgentCfg.Disabled {
				agentRunner = NewAgent(app.cfg.AgentCfg, apiClient, newMeta.TopologyRevision, app.logger)
				go agentRunner.Run(ctx)
			}

			if !app.cfg.MetricsServerConfig.Disabled {
				metricsServer = NewMetricsServer(app.cfg.MetricsServerConfig, *app, meta, app.logger)
				go metricsServer.Listen()
			}

			if app.cfg.JobsConfig.Enabled {
				jobsRunner = jobs.New(app.cfg.JobsConfig, apiClient, getToken, app.logger)
				go jobsRunner.Run(ctx)
			}

			initialized = true
		} else if metaChanged {
			app.logger.Info("Changes detected within metadata")
			if agentRunner != nil {
				agentRunner.SetTopologyRevision(newMeta.TopologyRevision)
			}
			if metricsServer != nil {
				metricsServer.meta = meta
			}
			if jobsRunner != nil {
				jobsConfig := app.cfg.JobsConfig
				jobsConfig.S3Logger.BucketName = meta.S3Bucket
				jobsRunner.SetConfig(jobsConfig)
			}
		}
	}
	stopUIProxyAgent()
}

type iamTokenAuthenticator struct {
	wrapped  http.RoundTripper
	getToken func() string
}

func (rt *iamTokenAuthenticator) RoundTrip(r *http.Request) (*http.Response, error) {
	iamToken := rt.getToken()
	r.Header.Add("Authorization", "Bearer "+iamToken)
	return rt.wrapped.RoundTrip(r)
}

func (app *App) runProxyAgent(ctx context.Context, getToken func() string) error {
	if app.cfg.UIProxyConfig.ProxyServerURL == "" {
		app.logger.Info("UI proxy component is disabled (proxy_server_url is empty)")
		return nil
	}

	authenticator := func(wrapped http.RoundTripper) http.RoundTripper {
		return &iamTokenAuthenticator{
			wrapped:  wrapped,
			getToken: getToken,
		}
	}
	agent, err := proxyagent.New(app.cfg.UIProxyConfig, authenticator, app.logger, nil)
	if err != nil {
		return err
	}
	go agent.Run(ctx)
	return nil
}

func (app *App) ensureKnoxState(shouldBeRunning bool) {
	err := app.execWithTimeout("service knox status")
	running := err == nil
	app.logger.Infof("Service knox expected state: %t, current state: %t", shouldBeRunning, running)
	if shouldBeRunning != running {
		if shouldBeRunning {
			app.enableKnox()
			return
		}
		app.disableKnox()
	}
}

func (app *App) enableKnox() {
	if app.cfg.AgentCfg.Systemd {
		// Dataproc Images 2.x uses ubuntu 20.04 with systemd and individual system user for dataproc-agent.
		_ = app.execWithTimeout("sudo /bin/systemctl start knox")
		_ = app.execWithTimeout("sudo /bin/systemctl enable knox")
	} else {
		// Dataproc Images 1.x uses ubuntu 16.04 with syv and root user for dataproc-agent.
		// It is required to run `update-rc.d knox defaults` at least once before running `enable` or `disable`.
		// For simplicity we run `defaults` each time. When turning knox off we run `defaults`+`disable`.
		// When turning knox on we run `defaults` only because it is actually equivalent to `enable`.
		_ = app.execWithTimeout("service knox start")
		_ = app.execWithTimeout("update-rc.d knox defaults")
	}
}

func (app *App) disableKnox() {
	if app.cfg.AgentCfg.Systemd {
		_ = app.execWithTimeout("sudo /bin/systemctl stop knox")
		_ = app.execWithTimeout("sudo /bin/systemctl disable knox")
	} else {
		_ = app.execWithTimeout("service knox stop")
		_ = app.execWithTimeout("update-rc.d knox defaults")
		_ = app.execWithTimeout("update-rc.d knox disable")
	}
}

func (app *App) execWithTimeout(command string) error {
	parts := strings.Split(command, " ")
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()
	err := exec.CommandContext(ctx, parts[0], parts[1:]...).Run()
	if err != nil {
		app.logger.Errorf("Command %q failed: %s", command, err)
	} else {
		app.logger.Infof("Command %q succeeded", command)
	}
	return err
}

func (app *App) metaStream() chan MetaInfo {
	c := make(chan MetaInfo, 1)
	if app.cfg.FakeMetaInfo.ServiceAccountToken != "" {
		c <- app.cfg.FakeMetaInfo
		return c
	}
	go func() {
		ticker := time.NewTicker(app.cfg.MetaTimeout)
		defer ticker.Stop()
		for range ticker.C {
			ctx, cancel := context.WithTimeout(context.Background(), app.cfg.MetaTimeout)
			newMeta, err := app.fetchMetadata(ctx)
			cancel()
			if err != nil {
				app.logger.Errorf("Failed to get meta: %s", err)
				continue
			}

			c <- newMeta
		}
	}()
	return c
}
