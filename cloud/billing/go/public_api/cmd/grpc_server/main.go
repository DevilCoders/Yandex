package main

import (
	"context"
	"os"

	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/config"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	grpcserver "a.yandex-team.ru/cloud/billing/go/public_api/internal/grpc_server"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/logging"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/scope"
	statuserver "a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/status_server"
)

var configPath string

func main() {
	cmd := cobra.Command{
		Use: "run",
		Run: runCommand,
	}

	cmd.SetOut(os.Stdout)
	cmd.SetErr(os.Stderr)

	cmd.PersistentFlags().StringVarP(&configPath, "config", "c", "", "Path to the config file")
	_ = cmd.MarkPersistentFlagRequired("config")

	if err := cmd.Execute(); err != nil {
		os.Exit(1)
	}
}

func runCommand(cmd *cobra.Command, args []string) {
	ctx := context.Background()

	cfg, err := config.LoadConfig(ctx, configPath)
	if err != nil {
		cmd.PrintErr(err)
		return
	}

	logger, err := logging.InitLogger(cfg)
	scope.StartGlobal(logger)
	defer scope.FinishGlobal()

	if err != nil {
		cmd.PrintErr(err)
		return
	}

	consoleClient := console.NewClient(cfg.ConsoleURL)

	grpcServer, err := grpcserver.Create(ctx, cfg)
	if err != nil {
		cmd.PrintErr(err)
		return
	}
	grpcserver.ConfigureServices(grpcServer, consoleClient)

	statusServer := statuserver.Create(cfg)

	wg, waitCtx := errgroup.WithContext(ctx)
	wg.Go(func() error { return grpcserver.Run(waitCtx, grpcServer, cfg.ListenAddr) })
	wg.Go(func() error { return statuserver.Run(waitCtx, statusServer) })

	<-waitCtx.Done()
	if err := wg.Wait(); err != nil {
		cmd.PrintErrln(err)
	}
}
