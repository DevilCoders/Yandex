package configuration

import (
	"os"

	"github.com/mitchellh/mapstructure"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
	"a.yandex-team.ru/transfer_manager/go/pkg/config"
	"a.yandex-team.ru/transfer_manager/go/pkg/token"
)

type PgConfig struct {
	User         config.Secret
	Password     config.Secret
	Database     string
	MDBClusterID string `mapstructure:"mdb_cluster_id"`
}

type Config struct {
	GrpcPort             int                  `mapstructure:"grpc_port"`
	HTTPPort             int                  `mapstructure:"http_port"`
	TokenProvider        token.ProviderConfig `mapstructure:"token_provider"`
	AuthProvider         auth.ProviderConfig  `mapstructure:"auth_provider"`
	Pg                   PgConfig
	MDBAPIURLBase        string            `mapstructure:"mdb_api_url_base"`
	YavToken             config.Secret     `mapstructure:"yav_token"`
	CloudCreds           config.CloudCreds `mapstructure:"cloud_creds"`
	CloudAPIEndpoint     string            `mapstructure:"cloud_api_endpoint"`
	KMSKeyID             string            `mapstructure:"kms_key_id"`
	KMSDiscoveryEndpoint string            `mapstructure:"kms_discovery_endpoint"`
}

func LoadConfig(configPath string) (*Config, error) {
	configFile, err := os.Open(configPath)
	if err != nil {
		return nil, xerrors.Errorf("unable to read config file: %w", err)
	}
	configMap := map[interface{}]interface{}{}
	yamlDecoder := yaml.NewDecoder(configFile)
	if err := yamlDecoder.Decode(configMap); err != nil {
		return nil, xerrors.Errorf("unable to parse yaml: %w", err)
	}

	c := new(Config)
	var deferredSecrets []config.DeferredSecret

	decoderConfig := mapstructure.DecoderConfig{DecodeHook: config.MakeConfigDecodeHook(&deferredSecrets), Result: c}
	decoder, err := mapstructure.NewDecoder(&decoderConfig)
	if err != nil {
		return nil, xerrors.Errorf("unable to create config decoder: %w", err)
	}

	if err := decoder.Decode(configMap); err != nil {
		return nil, xerrors.Errorf("unable to decode config: %w", err)
	}

	yavFactory := func() (config.YavSecretResolver, error) {
		return config.NewYavResolver(string(c.YavToken))
	}
	kmsFactory := func() (config.KMSSecretResolver, error) {
		return config.NewKMSResolver(c.CloudCreds, c.CloudAPIEndpoint, c.KMSKeyID, c.KMSDiscoveryEndpoint)
	}
	awsFactory := func() (config.AWSSecretResolver, error) {
		return config.NewKMSResolver(c.CloudCreds, c.CloudAPIEndpoint, c.KMSKeyID, c.KMSDiscoveryEndpoint)
	}
	lockboxFactory := func() (config.LockboxSecretResolver, error) {
		return config.NewLockboxResolver(c.CloudCreds, c.CloudAPIEndpoint)
	}
	if err := config.ResolveSecrets(deferredSecrets, yavFactory, kmsFactory, awsFactory, lockboxFactory); err != nil {
		return nil, xerrors.Errorf("unable to resolve secrets: %w", err)
	}

	return c, nil
}
