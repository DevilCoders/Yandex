package main

import (
	"context"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/monrun/runner"
	"a.yandex-team.ru/cloud/mdb/katan/monitoring/pkg/monitoring"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	warnSince  time.Duration
	critSince  time.Duration
	configPath string
)

func init() {
	flags := pflag.NewFlagSet("Zombie", pflag.ExitOnError)
	flags.StringVar(&configPath, "config", monitoring.ConfigPath, "Path to config")
	flags.DurationVar(&warnSince, "warn", time.Hour*25, "Warn level")
	flags.DurationVar(&critSince, "crit", time.Hour*50, "Crit level")
	pflag.CommandLine.AddFlagSet(flags)
}

func main() {
	pflag.Parse()
	runner.RunCheck(func(ctx context.Context, logger log.Logger) monrun.Result {
		m, err := monitoring.NewFromConfig(logger, configPath)
		if err != nil {
			return monrun.Warnf("initialization failed: %s", err)
		}
		return m.CheckZombieRollouts(ctx, warnSince, critSince)
	})
}
