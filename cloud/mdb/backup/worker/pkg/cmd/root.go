package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/flags"
)

var (
	clusterID     string
	clusterIDFlag = "cluster-id"
)

var (
	rootCmd = &cobra.Command{
		Use:   "mdb-backup-cli",
		Short: "Manages backup service for single or multiple clusters",
	}
)

func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}

func init() {
	flags.RegisterConfigPathFlagGlobal()
}

func SetClusterIDFlagRequired(cmd *cobra.Command) {
	cmd.PersistentFlags().StringVar(&clusterID, clusterIDFlag, "", "Cluster ID")
	if err := cobra.MarkFlagRequired(cmd.PersistentFlags(), clusterIDFlag); err != nil {
		panic(err)
	}
}
