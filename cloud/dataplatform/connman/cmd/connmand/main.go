package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
)

const (
	exitError = 1
)

func main() {
	var cmd = &cobra.Command{
		Use:           "connmand <command> <subcommand> [flags]",
		Short:         "Connection Manager Daemon",
		Long:          "Connection Manager allows to store connection info without hassle.",
		SilenceUsage:  true,
		SilenceErrors: true,
		Example: heredoc.Doc(`
			$ connman server start
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
	}

	cmd.AddCommand(serverCommand())

	if err := cmd.Execute(); err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		os.Exit(exitError)
	}
}
