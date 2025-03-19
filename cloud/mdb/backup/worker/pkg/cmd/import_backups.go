package cmd

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/importer"
	importcli "a.yandex-team.ru/cloud/mdb/backup/worker/pkg/cli"
)

const clusterTypesFlag = "cluster-types"

var (
	skipSchedDateDups = false
	completeFailed    = false
	dryRun            = true
	rawClusterTypes   []string
	batchSize         int
	importInterval    string
)

var importBackupsCmd = &cobra.Command{
	Use:   "import_backups [--dry-run]",
	Short: "Imports backup from storage",
	Args:  cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		cli, lg, err := importcli.NewAppFromConfig(ctx)
		if err != nil {
			fmt.Fprintf(os.Stderr, "%+v\n", err.Error())
			os.Exit(2)
		}

		var stats importer.ImportStats
		switch {
		case clusterID != "":
			stats, err = cli.ImportBackups(ctx, clusterID, skipSchedDateDups, completeFailed, dryRun)
		case len(rawClusterTypes) > 0:
			stats, err = runBatchImport(ctx, cli, batchSize)
		default:
			fmt.Fprintf(os.Stderr, "--%s or --%s must be specified", clusterID, clusterTypesFlag)
			os.Exit(2)
		}

		lg.Infof("Import statistics: %+v", stats)
		if err != nil {
			lg.Fatalf("failed to import backups: %+v", err)
		}
		lg.Info("Import has been completed successfully")
	},
}

func runBatchImport(ctx context.Context, cli *importcli.Cli, batchSize int) (importer.ImportStats, error) {
	clusterTypes, err := metadb.ClusterTypesFromStrings(rawClusterTypes)
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to parse cluster-types: %+v\n", err)
		os.Exit(3)
	}

	interval, err := time.ParseDuration(importInterval)
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to parse import interval: %+v\n", err)
		os.Exit(3)
	}

	return cli.BatchImportBackups(ctx, clusterTypes, batchSize, interval, skipSchedDateDups, completeFailed, dryRun)
}

func init() {
	importBackupsCmd.PersistentFlags().BoolVar(&dryRun, "dry-run", true, "do not insert backups into metadb")
	importBackupsCmd.PersistentFlags().BoolVar(&skipSchedDateDups, "skip-sched-date-dups", false, "skip backup import if violates unique key constraint")
	importBackupsCmd.PersistentFlags().BoolVar(&completeFailed, "complete-failed", false, "complete existing backup if it has failed status")
	importBackupsCmd.PersistentFlags().StringVar(&clusterID, clusterIDFlag, "", "Cluster ID")
	importBackupsCmd.PersistentFlags().IntVar(&batchSize, "batch-size", 200, "process no more than N clusters in a single run")
	importBackupsCmd.PersistentFlags().StringVar(&importInterval, "interval", "24h", "minimal interval between two subsequent import operations on a single cluster")
	importBackupsCmd.PersistentFlags().StringSliceVar(&rawClusterTypes, clusterTypesFlag, rawClusterTypes, "import provided cluster types")
	rootCmd.AddCommand(importBackupsCmd)
}
