package config

import "a.yandex-team.ru/transfer_manager/go/pkg/config"

//Config Server config
type Config struct {
	BaseURL  string        `mapstructure:"base_path"`
	HTTPPort string        `mapstructure:"http_port" default:":8080"`
	Proxy    string        `mapstructure:"yt_proxy" default:"hahn"`
	YTToken  config.Secret `mapstructure:"yt_token"`

	YavToken             config.Secret     `mapstructure:"yav_token"`
	CloudCreds           config.CloudCreds `mapstructure:"cloud_creds"`
	KMSKeyID             string            `mapstructure:"kms_key_id"`
	KMSDiscoveryEndpoint string            `mapstructure:"kms_discovery_endpoint"`
	MDBURLBase           string            `mapstructure:"mdb_api_url_base"`
	IAMURLBase           string            `mapstructure:"iam_api_url_base"`
	MDBToken             config.Secret     `mapstructure:"mdb_token"`
	CloudAPIEndpoint     string            `mapstructure:"cloud_api_endpoint"`
}
