package grpcserver

import (
	"os"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/alerting"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/duty"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/duty/config"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/conductor/httpapi"
	dbmapi "a.yandex-team.ru/cloud/mdb/internal/dbm/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
)

type Config struct {
	App          app.Config              `json:"app" yaml:"app"`
	Retry        retry.Config            `json:"retry" yaml:"retry"`
	GRPC         grpcutil.ServeConfig    `json:"grpc" yaml:"grpc"`
	SLBCloseFile string                  `json:"slb_close_file" yaml:"slb_close_file"`
	Auth         AuthConfig              `json:"auth" yaml:"auth"`
	IsCompute    bool                    `json:"is_compute" yaml:"is_compute"`
	UIHost       string                  `json:"ui_host" yaml:"ui_host"`
	Dbm          dbmapi.Config           `json:"dbm" yaml:"dbm"`
	Conductor    httpapi.ConductorConfig `json:"conductor" yaml:"conductor"`
	Cms          CmsConfig               `json:"cms" yaml:"cms"`
	FQDNSuffixes FQDNSuffixes            `json:"fqdn_suffixes" yaml:"fqdn_suffixes"`
	FQDN         string                  `json:"fqdn" yaml:"fqdn"`

	ConductorCacheRefreshInterval time.Duration `json:"conductor_cache_refresh_interval" yaml:"conductor_cache_refresh_interval"`
	ConductorProject              string        `json:"conductor_project" yaml:"conductor_project"`
}

type CmsConfig struct {
	Dom0Discovery duty.CmsDom0DutyConfig `json:"dom0_discovery" yaml:"dom0_discovery"`
	Alerting      alerting.AlertConfig   `json:"alerting" yaml:"alerting"`
}

type FQDNSuffixes struct {
	Controlplane       string `json:"controlplane" yaml:"controlplane"`
	UnmanagedDataplane string `json:"unmanaged_dataplane" yaml:"unmanaged_dataplane"`
	ManagedDataplane   string `json:"managed_dataplane" yaml:"managed_dataplane"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() *Config {
	fqdn, _ := os.Hostname()
	cfg := &Config{
		App:          app.DefaultConfig(),
		Retry:        retry.DefaultConfig(),
		GRPC:         grpcutil.DefaultServeConfig(),
		SLBCloseFile: "/tmp/.mdb-cms-close",
		Auth:         DefaultAuthConfig(),
		Dbm:          dbmapi.DefaultConfig(),
		Conductor:    httpapi.DefaultConductorConfig(),
		FQDNSuffixes: FQDNSuffixes{},
		Cms: CmsConfig{
			Dom0Discovery: duty.DefaultConfig(),
			Alerting:      alerting.DefaultAlertConfig(),
		},
		ConductorCacheRefreshInterval: time.Minute,
		ConductorProject:              "mdb",
		FQDN:                          fqdn,
	}
	cfg.App.Tracing.ServiceName = config.AppCfgName
	return cfg
}

// AuthConfig describes accessservice client configuration
type AuthConfig struct {
	Addr           string                `json:"addr" yaml:"addr"`
	Permission     string                `json:"permission" yaml:"permission"`
	FolderID       string                `json:"folder_id" yaml:"folder_id"`
	ClientConfig   grpcutil.ClientConfig `json:"config" yaml:"config"`
	SkipAuthErrors bool                  `json:"skip_auth_errors" yaml:"skip_auth_errors"`
}

// DefaultAuthConfig returns default AuthConfig
func DefaultAuthConfig() AuthConfig {
	return AuthConfig{
		ClientConfig: grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{CAFile: "/opt/yandex/allCAs.pem"},
			},
		},
	}
}
