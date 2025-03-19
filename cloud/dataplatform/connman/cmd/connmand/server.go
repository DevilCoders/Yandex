package main

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/dataplatform/connman/internal/configuration"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/server"
	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/config"
	"a.yandex-team.ru/transfer_manager/go/pkg/dbaas"
	ycsdk "a.yandex-team.ru/transfer_manager/go/pkg/yc/sdk"
)

func serverCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:     "server <command>",
		Aliases: []string{"s"},
		Short:   "connman server",
		Long:    "connman server commands.",
		Example: heredoc.Doc(`
			$ connman server start
			$ connman server start -c ./config.yaml
			$ connman server migrate
			$ connman server migrate -c ./config.yaml
		`),
	}

	cmd.AddCommand(startCommand())
	cmd.AddCommand(migrateCommand())

	return cmd
}

func startCommand() *cobra.Command {
	var configFile string

	cmd := &cobra.Command{
		Use:     "start",
		Aliases: []string{"s"},
		Short:   "Start the server",
		RunE: func(cmd *cobra.Command, args []string) error {
			ctx, cancel := context.WithCancel(context.Background())
			defer cancel()
			cfg, err := configuration.LoadConfig(configFile)
			if err != nil {
				return xerrors.Errorf("unable to load config: %w", err)
			}

			if err = initializeYC(cfg); err != nil {
				return xerrors.Errorf("unable to initialize yc: %w", err)
			}

			logger.Log.Info("cfg", log.Any("cfg", cfg))
			return server.Start(ctx, cfg, logger.Log)
		},
	}

	cmd.Flags().StringVarP(&configFile, "config", "c", "./config.yaml", "Config file path")
	return cmd
}

func initializeYC(cfg *configuration.Config) error {
	ycCredentials, err := config.ToYCCredentials(cfg.CloudCreds)
	if err != nil {
		return xerrors.Errorf("unable to get yc credentials: %w", err)
	}
	if err := ycsdk.InitializeWithCreds(cfg.CloudAPIEndpoint, ycCredentials); err != nil {
		return xerrors.Errorf("unable to initialize yc sdk: %w", err)
	}

	if err := dbaas.InitializeExternalCloud(cfg.MDBAPIURLBase); err != nil {
		return xerrors.Errorf("unable to initialize external cloud: %w", err)
	}

	return nil
}

func migrateCommand() *cobra.Command {
	var configFile string

	cmd := &cobra.Command{
		Use:   "migrate",
		Short: "Run database migrations",
		RunE: func(cmd *cobra.Command, args []string) error {
			logger.Log.Info("todo: implement me after contrib merged - CONTRIB-2557")
			return nil
		},
	}

	cmd.Flags().StringVarP(&configFile, "config", "c", "./config.yaml", "Config file path")
	return cmd
}
