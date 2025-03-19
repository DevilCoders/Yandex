package config

import (
	"time"

	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
	"a.yandex-team.ru/transfer_manager/go/pkg/config"
)

//GRPCConfig grpc options
type GRPCConfig struct {
	MaxRecvMsgSizeInMB int `mapstructure:"max_recv_msg_size_in_mb" default:"10"`
	MaxSendMsgSizeInMB int `mapstructure:"max_send_msg_size_in_mb" default:"10"`
}

type PgConfig struct {
	User     config.Secret      `mapstructure:"user"`
	Password config.Secret      `mapstructure:"password"`
	Database string             `mapstructure:"database"`
	Discover config.DBDiscovery `mapstructure:"discovery"`
}

//Config Server config
type Config struct {
	GRPCPort                    string               `mapstructure:"port" default:"8080"`
	HTTPPort                    string               `mapstructure:"http_port" default:"8182"`
	Timeout                     time.Duration        `mapstructure:"timeout" default:"60s"`
	CacheSizeInMB               int64                `mapstructure:"cache_size_in_mb" default:"100"`
	GRPC                        GRPCConfig           `mapstructure:"grpc"`
	Log                         config.Log           `mapstructure:"log"`
	DB                          PgConfig             `mapstructure:"db"`
	YavToken                    config.Secret        `mapstructure:"yav_token"`
	CloudCreds                  config.CloudCreds    `mapstructure:"cloud_creds"`
	MDBURLBase                  string               `mapstructure:"mdb_api_url_base"`
	IAMURLBase                  string               `mapstructure:"iam_api_url_base"`
	MDBToken                    config.Secret        `mapstructure:"mdb_token"`
	CloudAPIEndpoint            string               `mapstructure:"cloud_api_endpoint"`
	KMSKeyID                    string               `mapstructure:"kms_key_id"`
	KMSDiscoveryEndpoint        string               `mapstructure:"kms_discovery_endpoint"`
	AuthProvider                *auth.ProviderConfig `mapstructure:"auth_provider"`
	TLSServerPrivateCertificate config.Secret        `mapstructure:"tls_server_private_certificate"`
}
