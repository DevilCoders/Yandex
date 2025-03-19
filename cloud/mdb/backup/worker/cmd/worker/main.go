package main

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/backup/worker/pkg/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	showHelp = false
)

func init() {
	pflag.BoolVarP(&showHelp, "help", "h", false, "Show help message")
	flags.RegisterConfigPathFlagGlobal()
	pflag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage of %s:\n", os.Args[0])
		pflag.PrintDefaults()
	}
}

func main() {
	pflag.Parse()
	if showHelp {
		pflag.Usage()
		return
	}

	ctx := signals.WithCancelOnSignal(context.Background())
	worker, logger, err := app.NewAppFromConfig(ctx)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to start app: %+v\n", err)
		os.Exit(2)
	}
	logger.Debug("Starting worker")
	if err := worker.Run(ctx); err != nil {
		logger.Error("Worker exited with error", log.Error(err))
		os.Exit(3)
	}
	logger.Debug("Workers exited successfully.")
}
