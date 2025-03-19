package main

import (
	"context"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/monrun/runner"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/app"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	warnSince = time.Minute * 15
	critSince = time.Minute * 30
)

func init() {
	flags := pflag.NewFlagSet("BrokenSchedules", pflag.ExitOnError)
	flags.DurationVarP(&warnSince, "warn", "w", warnSince, "")
	flags.DurationVarP(&critSince, "crit", "c", critSince, "")
	pflag.CommandLine.AddFlagSet(flags)
}

func main() {
	pflag.Parse()
	runner.RunCheck(func(ctx context.Context, logger log.Logger) monrun.Result {
		return app.Monitor(ctx, warnSince, critSince, logger)
	})
}
