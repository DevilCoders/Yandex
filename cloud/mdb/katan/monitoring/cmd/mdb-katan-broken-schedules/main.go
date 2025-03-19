package main

import (
	"context"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/monrun/runner"
	"a.yandex-team.ru/cloud/mdb/katan/monitoring/pkg/monitoring"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	namespace  string
	configPath string
	uiURI      string
)

func init() {
	flags := pflag.NewFlagSet("BrokenSchedules", pflag.ExitOnError)
	flags.StringVar(&configPath, "config", monitoring.ConfigPath, "Path to config")
	flags.StringVar(&namespace, "namespace", "", "Schedule namespace")
	flags.StringVar(&uiURI, "ui-uri", "", "MDB UI URI")
	pflag.CommandLine.AddFlagSet(flags)
}

func main() {
	pflag.Parse()
	runner.RunCheck(func(ctx context.Context, logger log.Logger) monrun.Result {
		m, err := monitoring.NewFromConfig(logger, configPath)
		if err != nil {
			return monrun.Warnf("initialization failed: %s", err)
		}
		return m.CheckBrokenSchedules(ctx, namespace, uiURI)
	})
}
