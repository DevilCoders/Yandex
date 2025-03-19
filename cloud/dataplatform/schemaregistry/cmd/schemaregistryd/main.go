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
		Use:           "schemaregistry <command> <subcommand> [flags]",
		Short:         "Schema registry",
		Long:          "Schema registry to manage schemas efficiently.",
		SilenceUsage:  true,
		SilenceErrors: true,
		Example: heredoc.Doc(`
			$ schemaregistry server start
			$ schemaregistry serve migrate
		`),
	}

	cmd.AddCommand(ServerCommand())

	if err := cmd.Execute(); err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		os.Exit(exitError)
	}
}
