package main

import (
	"context"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/app"
	xlog "a.yandex-team.ru/library/go/core/log"
)

const initTimeout = 60 * time.Second

var autorecreateWorkerCmd = &cobra.Command{
	Use:   "autorecreate-worker",
	Short: "Run License Server Autorecreate Worker",
	Run:   autorecreateWorker,
}

var workerConfigsPath []string

func init() {
	autorecreateWorkerCmd.PersistentFlags().StringArrayVarP(
		&workerConfigsPath, "config", "c", nil, "service config files paths",
	)

	if err := autorecreateWorkerCmd.MarkPersistentFlagRequired("config"); err != nil {
		log.Fatal(err)
	}
}

func autorecreateWorker(_ *cobra.Command, _ []string) {
	baseCtx := context.Background()
	initCtx, initCancel := context.WithTimeout(baseCtx, initTimeout)
	defer initCancel()

	a, err := app.NewWorker(initCtx, workerConfigsPath)
	if err != nil {
		log.Fatal(err)
	}

	a.DefaultLogger().Info("running service")

	group, ctx := errgroup.WithContext(baseCtx)
	groupCtx, groupCancel := context.WithCancel(ctx)

	sigCh := make(chan os.Signal, 1)
	defer close(sigCh)

	signal.Notify(sigCh, os.Interrupt, syscall.SIGTERM)

	group.Go(func() error {
		return a.Run(groupCtx)
	})

	group.Go(func() error {
		<-sigCh

		a.DefaultLogger().Info("shutting down the server")
		groupCancel()
		return groupCtx.Err()
	})

	err = group.Wait()
	a.DefaultLogger().Info("exiting service", xlog.Error(err))
}
