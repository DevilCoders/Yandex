package config

import (
	"os"

	"github.com/mitchellh/mapstructure"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/config"
	"a.yandex-team.ru/transfer_manager/go/pkg/dbaas"
)

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
	if err := dbaas.InitializeInternalCloud(c.MDBURLBase, c.IAMURLBase, string(c.MDBToken)); err != nil {
		return nil, xerrors.Errorf("unable to init cloud provider: %w", err)
	}
	return c, nil
}
