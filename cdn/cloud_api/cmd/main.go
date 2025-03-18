package main

import (
	"context"
	"fmt"
	stdlog "log"
	"math/rand"
	"time"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/file"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cdn/cloud_api/pkg/application"
	"a.yandex-team.ru/cdn/cloud_api/pkg/configuration"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/collect"
	"a.yandex-team.ru/library/go/core/metrics/collect/policy/inflight"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

// TODO: Idempotence Key

func main() {
	rand.Seed(time.Now().UnixNano())

	ctx := context.Background()

	config, err := loadConfig(ctx)
	if err != nil {
		stdlog.Fatalf("failed to create config: %v", err)
	}

	logger, err := configuration.NewLogger(config.LoggerConfig)
	if err != nil {
		stdlog.Fatalf("failed to create logger: %v", err)
	}

	err = config.Validate()
	if err != nil {
		logger.Fatal("config validation failed", log.Error(err))
	}

	registry := newMetricRegistry(ctx)

	app := application.New(config, logger, registry)
	if err = app.Run(); err != nil {
		logger.Fatal("application stopped", log.Error(err))
	}
}

func loadConfig(ctx context.Context) (*configuration.Config, error) {
	config := configuration.NewDefaultConfig()

	configPath := pflag.StringP("config", "c", "", "config path")
	pflag.Parse()

	err := confita.NewLoader(file.NewBackend(*configPath)).Load(ctx, config)
	if err != nil {
		return nil, fmt.Errorf("load config: %w", err)
	}

	return config, nil
}

func newMetricRegistry(ctx context.Context) *solomon.Registry {
	registry := solomon.NewRegistry(
		solomon.NewRegistryOpts().
			SetRated(true).
			AddCollectors(ctx, inflight.NewCollectorPolicy(), collect.GoMetrics, collect.ProcessMetrics),
	)

	return registry
}
