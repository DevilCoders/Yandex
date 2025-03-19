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

	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/app"
	xlog "a.yandex-team.ru/library/go/core/log"
)

const initTimeout = 60 * time.Second

var productSyncerCmd = &cobra.Command{
	Use:   "product-sync",
	Short: "Run Product Syncer",
	Run:   runProductSyncerService,
}

var appConfigsPath []string

func init() {
	productSyncerCmd.PersistentFlags().StringArrayVarP(
		&appConfigsPath, "config", "c", nil, "syncer config files paths",
	)

	if err := productSyncerCmd.MarkPersistentFlagRequired("config"); err != nil {
		log.Fatal(err)
	}
}

func runProductSyncerService(cmd *cobra.Command, _ []string) {
	baseCtx := context.Background()
	initCtx, initCancel := context.WithTimeout(baseCtx, initTimeout)
	defer initCancel()

	a, err := app.NewApplication(initCtx, appConfigsPath)
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

		a.DefaultLogger().Info("shutting down product-syncer")
		groupCancel()
		return groupCtx.Err()
	})

	err = group.Wait()
	a.DefaultLogger().Info("exiting service", xlog.Error(err))
}
