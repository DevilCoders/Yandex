package cli

import (
	"context"
	"os"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/file"

	"a.yandex-team.ru/cloud/mdb/internal/tilde"
	intapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app/mdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	defaultConfigPath = "/etc/yandex/mdb-internal-api/mdb-internal-api.yaml"
)

// LoadConfig loads config from path
func LoadConfig(path string, logger log.Logger) (intapi.Config, error) {
	expanded, err := tilde.Expand(path)
	if err != nil {
		return intapi.Config{}, xerrors.Errorf("failed to expand config path: %w", err)
	}

	config := intapi.DefaultConfig()
	if _, err := os.Stat(expanded); os.IsNotExist(err) {
		logger.Warnf("failed to find config at path %q, using default config", expanded)
	} else {
		logger.Debugf("loading config from %q", expanded)
		loader := confita.NewLoader(file.NewBackend(expanded))

		if err = loader.Load(context.Background(), &config); err != nil {
			return intapi.Config{}, xerrors.Errorf("failed to parse config: %w", err)
		}
	}

	logger.Debugf("loaded config: %+v", config)
	return config, nil
}
