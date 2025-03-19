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
		Use:           "schemaregistryctl <command> <subcommand> [flags]",
		Short:         "Schema Registry CLI",
		Long:          "Schema Registry allows to admin schemas info without hassle.",
		SilenceUsage:  true,
		SilenceErrors: false,
		Example: heredoc.Doc(`
			$ schemaregistry namespace create
			$ schemaregistry schema create
		`),
	}

	cmd.AddCommand(NamespaceCmd())
	cmd.AddCommand(SchemaCmd())
	cmd.AddCommand(SearchCmd())
	cmd.AddCommand(ExtractCmd())

	if err := cmd.Execute(); err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		os.Exit(exitError)
	}
}
