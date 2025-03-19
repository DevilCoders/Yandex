package cmd

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
)

// New root command
func New() *cobra.Command {
	var cmd = &cobra.Command{
		Use:           "steadyprofiler <command> <subcommand> [flags]",
		Short:         "Continuous Go-Profiler",
		Long:          "Store and Serve go-profilers.",
		SilenceUsage:  true,
		SilenceErrors: true,
		Example: heredoc.Doc(`
			$ steadyprofiler server start
		`),
		Annotations: map[string]string{
			"group:core": "true",
			"help:learn": heredoc.Doc(`
				Use 'steadyprofiler <command> <subcommand> --help' for more information about a command.
				Read the manual at TODO
			`),
			"help:feedback": heredoc.Doc(`
				Open an issue here st/TM
			`),
		},
	}

	cmd.AddCommand(ServerCommand())

	return cmd
}
