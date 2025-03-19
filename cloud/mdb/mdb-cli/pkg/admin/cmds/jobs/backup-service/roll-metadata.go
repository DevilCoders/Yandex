package backupservice

import (
	"context"
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/helpers"
)

const (
	rollMetadataCommandName          = "roll-metadata"
	backupCliRollMetadataCommandName = "roll_metadata"
	ignoreHealthCheckFlag            = "ignore-health-check"
)

var (
	rollMetadataCmd = initRollMetadataCommand()

	ignoreHealthCheck = false
)

func initRollMetadataCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   fmt.Sprintf("%s [--%s]=<true|false>", rollMetadataCommandName, ignoreHealthCheckFlag),
		Short: "Roll metadata on cluster hosts.",
		Args:  cobra.NoArgs,
	}

	cmd.Flags().BoolVar(&ignoreHealthCheck, ignoreHealthCheckFlag, false, "ignore cluster hosts health")
	return &cli.Command{Cmd: cmd, Run: runRollMetadata}
}

func runRollMetadata(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	rollMetadataArgs := []string{
		fmt.Sprintf("--%s", clusterIDFlag),
		clusterID,
		fmt.Sprintf("--%s=%v", ignoreHealthCheckFlag, ignoreHealthCheck),
	}

	helpers.RunBackupWorkerJob(env, backupCliRollMetadataCommandName, rollMetadataArgs)
}
