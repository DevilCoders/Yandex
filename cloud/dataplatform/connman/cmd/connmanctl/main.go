package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

const (
	exitError = 1
)

func main() {
	var cmd = &cobra.Command{
		Use:           "connmanctl <command> <subcommand> [flags]",
		Short:         "Connection Manager Admin CLI",
		Long:          "Connection Manager allows to admin connection info without hassle.",
		SilenceUsage:  true,
		SilenceErrors: false,
	}

	cmd.AddCommand(connectionCmd())

	if err := cmd.Execute(); err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		os.Exit(exitError)
	}
}
