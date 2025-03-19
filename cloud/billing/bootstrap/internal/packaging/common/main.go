package main

import (
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/applier"
)

var cmd = cobra.Command{
	Use:               os.Args[0],
	CompletionOptions: cobra.CompletionOptions{DisableDefaultCmd: true},
}

func init() {
	cmd.AddCommand(
		applier.ExtractCmd(),
		applier.StatsCmd(),
	)
}

func main() {
	if err := cmd.Execute(); err != nil {
		os.Exit(1)
	}
}
