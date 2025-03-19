package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/cli"
)

func main() {
	var version string
	var quiet bool
	cfg := cli.DefaultConfig()

	var cmdRun = &cobra.Command{
		Use:   "run deploy-command [argument] ...",
		Short: "Run deploy command",
		Long:  `run given deploy command waiting for its execution out.`,
		Example: `	run state.highstate test 'pillar={service-restart: true}'
	run state.sls components.dbaas-operations.update-salt	`,
		Args: cobra.MinimumNArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			var cmdArgs []string
			if len(args) > 1 {
				cmdArgs = args[1:]
			}
			if err := cli.RunCommand(cfg, args[0], cmdArgs, version, quiet); err != nil {
				fmt.Printf("cli fails unexpectedly: %s", err)
				fmt.Println()
				os.Exit(1)
			}
		},
		ValidArgs: []string{
			"state.highstate",
			"state.sls",
			"pillar.get",
			"pillar.items",
			"saltutil.sync_all",
			"grains.items",
			"grains.get",
		},
	}
	cmdRun.Flags().StringVarP(&version, "version", "v", "", "srv version")
	cmdRun.Flags().BoolVarP(&quiet, "quiet", "q", false, "don't print deploy progress messages")

	var rootCmd = &cobra.Command{Use: "mdb-deploy [command]"}
	rootCmd.CompletionOptions.HiddenDefaultCmd = true
	rootCmd.AddCommand(cmdRun)
	rootCmd.PersistentFlags().StringVarP(&cfg.Addr, "address", "a", cfg.Addr, "deploy agent address")

	_ = rootCmd.Execute()
}
