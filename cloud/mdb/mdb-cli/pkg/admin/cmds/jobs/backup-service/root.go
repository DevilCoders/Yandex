package backupservice

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

const (
	clusterIDFlag = "cluster-id"
)

var (
	Cmd       = initCommand()
	clusterID string
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "backup-service",
		Short: "Manages backup service for given clusterID",
	}

	cmd.PersistentFlags().StringVar(&clusterID, clusterIDFlag, "", "Cluster ID")
	if err := cobra.MarkFlagRequired(cmd.PersistentFlags(), clusterIDFlag); err != nil {
		panic(err)
	}

	return &cli.Command{
		Cmd: cmd,
		SubCommands: []*cli.Command{
			importCmd,
			usageCmd,
			rollMetadataCmd,
		},
	}
}
