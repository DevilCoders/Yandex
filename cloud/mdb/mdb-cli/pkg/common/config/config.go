package config

import (
	"context"
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/file"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/tilde"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Defaults
const (
	DefaultConfigPath = "~/.mdb-cli"
	allCAFileName     = "allCAs.pem"
	gpnCAFileName     = "gpnCAs.pem"
)

// FormatConfigFilePath ...
func FormatConfigFilePath(p, name string) string {
	return path.Join(p, fmt.Sprintf("%s.yaml", name))
}

// FormatCAFilePath ...
func FormatAllCAFilePath(p string) string {
	return path.Join(p, allCAFileName)
}

// FormatGPNCAFilePath ...
func FormatGPNCAFilePath(p string) string {
	return path.Join(p, gpnCAFileName)
}

const (
	// Deployments config names
	DeploymentNameComputeProd    = "compute-prod"
	DeploymentNameComputePreprod = "compute-preprod"
	DeploymentNamePortoProd      = "porto-prod"
	DeploymentNamePortoTest      = "porto-test"
	DeploymentNameGpnProd        = "gpn-prod"
)

// LoadConfig loads config from path
func LoadConfig(path string, logger log.Logger) (Config, error) {
	ConfigPath, err := tilde.Expand(path)
	if err != nil {
		return Config{}, err
	}

	logger.Debugf("Loading config from %q", ConfigPath)
	loader := confita.NewLoader(file.NewBackend(ConfigPath))

	var vc versionedConfig
	if err = loader.Load(context.Background(), &vc); err != nil {
		return Config{}, xerrors.Errorf("failed to load version from config: %w", err)
	}

	cfg := DefaultConfig()
	if err = vc.Config(&cfg); err != nil {
		// Special handling of v0 config because its not versioned
		if !xerrors.Is(err, errUnknownVersion) {
			return Config{}, xerrors.Errorf("failed to parse config: %w", err)
		}

		var v0 V0
		if err = loader.Load(context.Background(), &v0); err != nil {
			return Config{}, xerrors.Errorf("failed to load config v0: %w", err)
		}

		logger.Info("Detected config v0. Upgrading...")
		cfg = v0.Upgrade()
	}

	if err = cfg.Validate(); err != nil {
		return Config{}, xerrors.Errorf("invalid config: %w", err)
	}

	if !vc.IsLatest() {
		logger.Infof("Storing config upgraded from v%d to v%d...", vc.Version, LatestVersion)
		if err = WriteConfig(cfg, ConfigPath); err != nil {
			logger.Errorf("failed to write upgraded config: %s", err)
		}
	}

	cfg.CAPath, err = tilde.Expand(cfg.CAPath)
	if err != nil {
		logger.Fatalf("Failed to expand path: %s", err)
	}

	if cfg.DefaultDeployment != "" {
		if _, ok := cfg.Deployments[cfg.DefaultDeployment]; !ok {
			logger.Fatalf(
				"Unknown default deployment %q, configured deployments: %s",
				cfg.DefaultDeployment,
				cfg.DeploymentNames(),
			)
		}
	}

	logger.Debugf("Loaded config: %+v", cfg)
	return cfg, nil
}

func IsValidDeployment(cfg Config, name string) error {
	for d := range cfg.Deployments {
		if name == d {
			return nil
		}
	}

	var ds []string
	for name := range cfg.Deployments {
		ds = append(ds, name)
	}

	return xerrors.Errorf("deployment %q is invalid, possible values are: %v", name, ds)
}

func WriteConfig(cfg Config, path string) error {
	p, err := tilde.Expand(path)
	if err != nil {
		return xerrors.Errorf("failed to expand %q: %s", path, err)
	}

	if err := os.MkdirAll(filepath.Dir(p), 0700); err != nil {
		return xerrors.Errorf("failed to create directories at path %q: %w", p, err)
	}

	versioned := latestVersionedConfig{
		Version: LatestVersion,
		Data:    cfg,
	}

	marshaled, err := yaml.Marshal(versioned)
	if err != nil {
		return xerrors.Errorf("failed to marshal config: %w", err)
	}

	if err := ioutil.WriteFile(p, marshaled, 0600); err != nil {
		return xerrors.Errorf("failed to write config to %q: %w", p, err)
	}

	return nil
}

// FromEnv returns Config stored in env
func FromEnv(e *cli.Env) Config {
	return *e.Config.(*Config)
}
