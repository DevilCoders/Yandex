package config

import (
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	errUnknownVersion = xerrors.NewSentinel("unknown config version")
)

type Config = V7
type DeploymentConfig = DeploymentV7
type DeploymentFederation = DeploymentFederationV7
type DeploymentToken = DeploymentTokenV7

var LatestVersion = 7

type latestVersionedConfig struct {
	Version int    `yaml:"version"`
	Data    Config `yaml:"data"`
}

type versionedConfig struct {
	Version int    `yaml:"version"`
	Data    []byte `yaml:"data"`
}

func (vc *versionedConfig) IsLatest() bool {
	return vc.Version == LatestVersion
}

func (vc *versionedConfig) Config(cfg *Config) error {
	// TODO: handle this better, right now its kinda bad
	switch vc.Version {
	case 7:
		if err := yaml.Unmarshal(vc.Data, cfg); err != nil {
			return xerrors.Errorf("failed to unmarshal v7 config: %w", err)
		}

		return nil
	case 6:
		var v6 V6
		if err := yaml.Unmarshal(vc.Data, &v6); err != nil {
			return xerrors.Errorf("failed to unmarshal v6 config: %w", err)
		}

		*cfg = v6.Upgrade()
		return nil
	case 5:
		var v5 V5
		if err := yaml.Unmarshal(vc.Data, &v5); err != nil {
			return xerrors.Errorf("failed to unmarshal v5 config: %w", err)
		}

		*cfg = v5.Upgrade()
		return nil
	case 4:
		var v4 V4
		if err := yaml.Unmarshal(vc.Data, &v4); err != nil {
			return xerrors.Errorf("failed to unmarshal v4 config: %w", err)
		}

		*cfg = v4.Upgrade()
		return nil
	case 3:
		var v3 V3
		if err := yaml.Unmarshal(vc.Data, &v3); err != nil {
			return xerrors.Errorf("failed to unmarshal v3 config: %w", err)
		}

		*cfg = v3.Upgrade()
		return nil
	case 2:
		var v2 V2
		if err := yaml.Unmarshal(vc.Data, &v2); err != nil {
			return xerrors.Errorf("failed to unmarshal v2 config: %w", err)
		}

		*cfg = v2.Upgrade()
		return nil
	case 1:
		var v1 V1
		if err := yaml.Unmarshal(vc.Data, &v1); err != nil {
			return xerrors.Errorf("failed to unmarshal v1 config: %w", err)
		}

		*cfg = v1.Upgrade()
		return nil
	default:
		return xerrors.Errorf("config version %d: %w", vc.Version, errUnknownVersion)
	}
}

func (vc *versionedConfig) UnmarshalYAML(unmarshal func(interface{}) error) error {
	var untyped struct {
		Version int                    `yaml:"version"`
		Data    map[string]interface{} `yaml:"data"`
	}
	if err := unmarshal(&untyped); err != nil {
		return err
	}

	vc.Version = untyped.Version
	var err error
	vc.Data, err = yaml.Marshal(untyped.Data)
	return err
}
