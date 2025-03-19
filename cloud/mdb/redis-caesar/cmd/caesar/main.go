package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/app"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config/flags"
)

//nolint: gochecknoglobals
var startFlags *flags.Root

//nolint: gochecknoglobals
var rootCmd = &cobra.Command{
	Use:   "caesar",
	Short: "Redis Caesar is a Redis HA cluster coordination tool",
	Long:  `Running without additional arguments will start caesar agent for current node.`,
	Run: func(cmd *cobra.Command, args []string) {
		os.Exit(app.Start(startFlags))
	},
}

// nolint: gochecknoinits
func init() {
	startFlags = flags.ConfigureRoot(rootCmd)
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
