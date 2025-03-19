package main

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/config"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/store/postgres"
	"a.yandex-team.ru/library/go/core/log"
)

func ServerCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:     "server <command>",
		Aliases: []string{"s"},
		Short:   "Server management",
		Long:    "Server management commands.",
		Example: heredoc.Doc(`
			$ schemaregistry server start
			$ schemaregistry server start -c ./config.yaml
			$ schemaregistry server migrate
			$ schemaregistry server migrate -c ./config.yaml
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
			cfg, err := config.LoadConfig(configFile)
			if err != nil {
				return err
			}
			logger.Log.Info("cfg", log.Any("cfg", cfg))
			server.Start(cfg)
			return nil
		},
	}

	cmd.Flags().StringVarP(&configFile, "config", "c", "./config.yaml", "Config file path")
	return cmd
}

func migrateCommand() *cobra.Command {
	var configFile string

	cmd := &cobra.Command{
		Use:   "migrate",
		Short: "Run database migrations",
		RunE: func(cmd *cobra.Command, args []string) error {
			cfg, err := config.LoadConfig(configFile)
			if err != nil {
				return err
			}

			if err := postgres.Migrate(cfg.DB); err != nil {
				return err
			}

			return nil
		},
	}

	cmd.Flags().StringVarP(&configFile, "config", "c", "./config.yaml", "Config file path")
	return cmd
}
