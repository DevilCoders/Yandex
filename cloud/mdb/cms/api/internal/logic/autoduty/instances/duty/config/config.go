package config

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/mwswitch"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/grpc"
)

const (
	AppCfgName = "mdb-cms-instance"
)

type FQDNSuffixes struct {
	Controlplane       string `json:"controlplane" yaml:"controlplane"`
	UnmanagedDataplane string `json:"unmanaged_dataplane" yaml:"unmanaged_dataplane"`
	ManagedDataplane   string `json:"managed_dataplane" yaml:"managed_dataplane"`
}

type Config struct {
	Mlock              grpc.Config              `json:"mlock" yaml:"mlock"`
	App                app.Config               `json:"app" yaml:"app"`
	Health             swagger.Config           `json:"health" yaml:"health"`
	AutoDuty           AutoDutyConfig           `json:"auto_duty" yaml:"auto_duty"`
	Juggler            http.Config              `json:"juggler" yaml:"juggler"`
	Deploy             restapi.Config           `json:"deploy" yaml:"deploy"`
	IsCompute          bool                     `json:"is_compute" yaml:"is_compute"`
	MaxConcurrentTasks int                      `json:"max_concurrent_tasks" yaml:"max_concurrent_tasks"`
	FQDNSuffixes       FQDNSuffixes             `json:"fqdn_suffixes" yaml:"fqdn_suffixes"`
	EnabledMW          mwswitch.EnabledMWConfig `json:"enabled_mw" yaml:"enabled_mw"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	cfg := Config{
		App:                app.DefaultConfig(),
		Juggler:            http.DefaultConfig(),
		Deploy:             restapi.DefaultConfig(),
		Mlock:              grpc.DefaultConfig(),
		AutoDuty:           DefaultAutoDutyConfig(),
		Health:             swagger.DefaultConfig(),
		IsCompute:          false,
		MaxConcurrentTasks: 20,
		FQDNSuffixes:       FQDNSuffixes{},
		EnabledMW:          mwswitch.EnabledMWConfig{},
	}
	cfg.App.Tracing.ServiceName = AppCfgName
	return cfg
}

type AutoDutyConfig struct {
	Cmsdb  pgutil.Config `json:"cmsdb" yaml:"cmsdb"`
	Metadb pgutil.Config `json:"metadb" yaml:"metadb"`
}

func DefaultAutoDutyConfig() AutoDutyConfig {
	return AutoDutyConfig{
		Cmsdb:  pgutil.DefaultConfig(),
		Metadb: pgutil.DefaultConfig(),
	}
}
