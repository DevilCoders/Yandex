package cmd

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
	"a.yandex-team.ru/cloud/dataplatform/steadyprofiler/internal/config"
	"a.yandex-team.ru/cloud/dataplatform/steadyprofiler/internal/server"
)

func ServerCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:     "server <command>",
		Aliases: []string{"s"},
		Short:   "Server management",
		Long:    "Server management commands.",
		Example: heredoc.Doc(`
			$ steadyprofiler server start <ListenURL> <BaseURL>
		`),
	}

	cmd.AddCommand(startCommand())

	return cmd
}

func startCommand() *cobra.Command {
	var configFile string

	cmd := &cobra.Command{
		Use:     "start <ListenURL> <BaseURL>",
		Aliases: []string{"s"},
		Short:   "Start the server",
		RunE: func(cmd *cobra.Command, args []string) error {
			cfg, err := config.Load(configFile)
			if err != nil {
				return err
			}
			return server.Start(cfg)
		},
	}

	cmd.Flags().StringVarP(&configFile, "config", "c", "./config.yaml", "Config file path")
	return cmd
}
