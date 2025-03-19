package config

import (
	"context"
	"log"
	"path/filepath"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/file"
)

func createConfigLoader(paths []string) *confita.Loader {
	var configFiles []string
	for i := range paths {
		paths, err := filepath.Glob(paths[i])
		if err != nil {
			log.Fatal(err)
		}

		configFiles = append(configFiles, paths...)
	}

	var backends []backend.Backend
	for _, path := range configFiles {
		backends = append(backends, file.NewBackend(path))
	}

	return confita.NewLoader(backends...)
}

// Load load service config from provided configuration files.
func Load(ctx context.Context, paths []string) (*ServiceConfig, error) {
	var cfg = new(ServiceConfig)

	if err := createConfigLoader(paths).Load(ctx, cfg); err != nil {
		return nil, err
	}

	return cfg, nil
}
