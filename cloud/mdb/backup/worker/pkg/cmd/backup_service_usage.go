package cmd

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/cobra"

	importcli "a.yandex-team.ru/cloud/mdb/backup/worker/pkg/cli"
)

var (
	enabled bool
)

var backupServiceUsageCmd = &cobra.Command{
	Use:   "backup_service_usage --enabled <true|false>",
	Short: "Set backup service usage",
	Args:  cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		cli, lg, err := importcli.NewAppFromConfig(ctx)
		if err != nil {
			fmt.Fprintf(os.Stderr, "%+v\n", err.Error())
			os.Exit(2)
		}

		if err := cli.SetBackupServiceEnabled(ctx, clusterID, enabled); err != nil {
			lg.Fatalf("failed to enable backup service: %+v\n", err)

		}
	},
}

func init() {
	backupServiceUsageCmd.PersistentFlags().BoolVar(&enabled, "enabled", true, "backup service should be enabled")
	SetClusterIDFlagRequired(backupServiceUsageCmd)
	rootCmd.AddCommand(backupServiceUsageCmd)
}
