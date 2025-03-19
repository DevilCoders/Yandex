package internal

import (
	"path"

	"cuelang.org/go/pkg/time"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/logbroker"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const ConfigFileName = "billing.yaml"
const StateFileName = "state.json"
const SendStateFileName = "send.state.json"
const MetricsLogFileName = "metrics.log"
const MetricVersion = "v1"

type BillingConfig struct {
	Schema        string                 `json:"schema" yaml:"schema"`
	LicenseSchema string                 `json:"license_schema" yaml:"license_schema"`
	FQDN          string                 `json:"fqdn" yaml:"fqdn"`
	CloudID       string                 `json:"cloud_id" yaml:"cloud_id"`
	FolderID      string                 `json:"folder_id" yaml:"folder_id"`
	ClusterID     string                 `json:"cluster_id" yaml:"cluster_id"`
	Tags          map[string]interface{} `json:"tags" yaml:"tags"`
	LicenseTags   map[string]interface{} `json:"license_tags" yaml:"license_tags"`
}

type Config struct {
	App                   app.Config            `json:"app" yaml:"app"`
	TelegrafStateFile     string                `json:"telegraf_state_file" yaml:"telegraf_state_file"`
	TelegrafChecks        map[string]string     `json:"telegraf_checks" yaml:"telegraf_checks"`
	TelegrafLicenseChecks map[string]string     `json:"telegraf_license_checks" yaml:"telegraf_license_checks"`
	SendLicenseMetrics    bool                  `json:"send_license_metrics" yaml:"send_license_metrics"`
	DataDir               string                `json:"data_dir" yaml:"data_dir"`
	MaxMetricsLogSize     int64                 `json:"max_metrics_log_size" yaml:"max_metrics_log_size"`
	LogBroker             logbroker.Config      `json:"logbroker" yaml:"logbroker"`
	SendTimeout           encodingutil.Duration `json:"send_timeout" yaml:"send_timeout"`
	RetryTimeout          encodingutil.Duration `json:"retry_timeout" yaml:"retry_timeout"`
	Billing               BillingConfig         `json:"billing" yaml:"billing"`
	ReportInterval        encodingutil.Duration `json:"report_interval" yaml:"report_interval"`
	PollInterval          encodingutil.Duration `json:"poll_interval" yaml:"poll_interval"`
	LockFile              string                `json:"lock_file" yaml:"lock_file"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func (c Config) StatePath() string {
	return path.Join(c.DataDir, StateFileName)
}

func (c Config) StateSendPath() string {
	return path.Join(c.DataDir, SendStateFileName)
}

func (c Config) MetricsLogPath() string {
	return path.Join(c.DataDir, MetricsLogFileName)
}

func DefaultConfig() Config {
	return Config{
		App: app.DefaultConfig(),
		Billing: BillingConfig{
			Schema:        "mdb.db.generic.v1",
			LicenseSchema: "mdb.db.license.v1",
		},
		SendLicenseMetrics: true,
		SendTimeout:        encodingutil.Duration{Duration: 30 * time.Second},
		RetryTimeout:       encodingutil.Duration{Duration: 30 * time.Second},
		ReportInterval:     encodingutil.Duration{Duration: 60 * time.Second},
		PollInterval:       encodingutil.Duration{Duration: 10 * time.Second},
	}
}

func ConfigureLogger(cfg app.LoggingConfig) (log.Logger, error) {
	zapConfig := zap.ConsoleConfig(cfg.Level)
	if cfg.File != "" {
		zapConfig.OutputPaths = []string{cfg.File}
	}
	return zap.New(zapConfig)
}
