package config

import (
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	AppCfgName       = "mdb-vpc-worker"
	VpcPasswdEnvName = "VPCDB_PASSWORD"
	AwsSecretEnvName = "AWS_SECRET_ACCESS_KEY"
)

type Config struct {
	App   app.Config    `json:"app" yaml:"app"`
	Aws   AwsConfig     `json:"aws" yaml:"aws"`
	Vpcdb pgutil.Config `json:"vpcdb" yaml:"vpcdb"`

	MaxConcurrentTasks int `json:"max_concurrent_tasks" yaml:"max_concurrent_tasks"`
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

type AwsConfig struct {
	Regions         []string              `json:"regions" yaml:"regions"`
	AccessKeyID     string                `json:"access_key_id" yaml:"access_key_id"`
	SecretAccessKey secret.String         `json:"secret_access_key" yaml:"secret_access_key"`
	HTTP            httputil.ClientConfig `json:"http" yaml:"http"`
	ControlPlane    AwsControlPlaneConfig `json:"control_plane" yaml:"control_plane"`
}

type AwsControlPlaneConfig struct {
	NlbNets              []string `json:"nlb_nets" yaml:"nlb_nets"`
	YandexnetsIPv4       []string `json:"yandexnets_ipv4" yaml:"yandexnets_ipv4"`
	YandexnetsIPv6       []string `json:"yandexnets_ipv6" yaml:"yandexnets_ipv6"`
	YandexProjectsNats   []string `json:"yandex_projects_nats" yaml:"yandex_projects_nats"`
	YandexProjectsNatsV6 []string `json:"yandex_projects_nats_v6" yaml:"yandex_projects_nats_v6"`
	OpenPorts            []int64  `json:"open_ports" yaml:"open_ports"`
	DNSZoneID            string   `json:"dns_zone_id" yaml:"dns_zone_id"`
	Name                 string   `json:"name" yaml:"name"`
	VersionID            string   `json:"version_id" yaml:"version_id"`
	OpenDataplanePorts   bool     `json:"open_dataplane_ports" yaml:"open_dataplane_ports"`

	TransitGateways    map[string]TransitGateway `json:"transit_gateways" yaml:"transit_gateways"`
	DNSRegionalMapping map[string]string         `json:"dns_regional_mapping" yaml:"dns_regional_mapping"`
	DataplaneRolesArns AwsDataplaneRolesArns     `json:"dataplane_roles_arns" yaml:"dataplane_roles_arns"`

	DiscountTag string `json:"discount_tag" yaml:"discount_tag"`
}

type AwsDataplaneRolesArns struct {
	ClickHouse string `json:"clickhouse" yaml:"clickhouse"`
}

type TransitGateway struct {
	ID           string `json:"id" yaml:"id"`
	RouteTableID string `json:"route_table_id" yaml:"route_table_id"`
	ShareARN     string `json:"share_arn" yaml:"share_arn"`
}

func DefaultAwsConfig() AwsConfig {
	return AwsConfig{
		HTTP: httputil.ClientConfig{
			Transport: httputil.DefaultTransportConfig(),
		},
	}
}

func DefaultConfig() Config {
	return Config{
		App:                app.DefaultConfig(),
		Aws:                DefaultAwsConfig(),
		Vpcdb:              pgutil.DefaultConfig(),
		MaxConcurrentTasks: 10,
	}
}

func LoadSecrets(cfg *Config, logger log.Logger) {
	if !cfg.Vpcdb.Password.FromEnv(VpcPasswdEnvName) {
		logger.Infof("%s is empty", VpcPasswdEnvName)
	}

	if !cfg.Aws.SecretAccessKey.FromEnv(AwsSecretEnvName) {
		logger.Infof("%s is empty", AwsSecretEnvName)
	}
}
