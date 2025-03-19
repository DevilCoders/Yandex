package config

import (
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/environment"
	"a.yandex-team.ru/cloud/mdb/internal/deprecated/app/swagger"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/core"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	das "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi/http"
)

const (
	MdbdnsConfigName  = "mdb-dns.yaml"
	MetadbConfigName  = "metadbpg.yaml"
	slayerDNSTokenVar = "SLAYERDNS_TOKEN"
)

type R53Config struct {
	HostedZoneID      string `json:"hosted_zone_id" yaml:"hosted_zone_id"`
	FQDNSuffix        string `json:"fqdnsuffix" yaml:"fqdnsuffix"`
	PrivateFQDNSuffix string `json:"private_fqdn_suffix" yaml:"private_fqdn_suffix"`
	MaxRec            uint   `json:"maxrec" yaml:"maxrec"`
	UpdThrs           uint   `json:"updthreads" yaml:"updthreads"`
}

// DNSAPIOptions ...
type DNSAPIOptions struct {
	Slayer  dnsapi.Config `json:"slayer" yaml:"slayer"`
	Compute dnsapi.Config `json:"compute" yaml:"compute"`
	Route53 R53Config     `json:"route53" yaml:"route53"`
}

type Config struct {
	swagger.Config `json:"config" yaml:"config,inline"`
	App            app.Config                 `json:"app" yaml:"app"`
	DMConf         core.DMConfig              `json:"dnsman" yaml:"dnsman"`
	DNSAPI         DNSAPIOptions              `json:"dnsapi" yaml:"dnsapi"`
	ServiceAccount app.ServiceAccountConfig   `json:"service_account" yaml:"service_account"`
	Environment    environment.EnvironmentCfg `json:"environment" yaml:"environment"`
	AppName        string                     `json:"app_name" yaml:"app_name"`
}

func DefaultConfig() Config {
	cfg := Config{
		App:    app.DefaultConfig(),
		Config: swagger.DefaultConfig(),
		DMConf: core.DefaultDMConfig(),
		DNSAPI: DNSAPIOptions{
			Slayer: das.DefaultConfig(),
		},
	}
	cfg.App.Tracing.ServiceName = "mdb-dns"
	cfg.DNSAPI.Slayer.Token.FromEnv(slayerDNSTokenVar)
	cfg.ServiceAccount.FromEnv("MDB_DNS")
	return cfg
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}
