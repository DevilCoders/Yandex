package cmd

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/cobra"

	importcli "a.yandex-team.ru/cloud/mdb/backup/worker/pkg/cli"
)

var (
	ignoreHealthCheck = false
)

var rollMetadataCmd = &cobra.Command{
	Use:   "roll_metadata [--ignore-health-check]",
	Short: "Roll metadata on cluster hosts",
	Args:  cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		cli, lg, err := importcli.NewAppFromConfig(ctx)
		if err != nil {
			fmt.Fprintf(os.Stderr, "%+v\n", err.Error())
			os.Exit(2)
		}
		if err := cli.RollMetadata(ctx, clusterID, ignoreHealthCheck); err != nil {
			lg.Fatalf("failed to roll metadata: %+v\n", err)
		}
	},
}

func init() {
	rollMetadataCmd.PersistentFlags().BoolVar(&ignoreHealthCheck, "ignore-health-check", false, "ignore cluster hosts health")
	SetClusterIDFlagRequired(rollMetadataCmd)
	rootCmd.AddCommand(rollMetadataCmd)
}
