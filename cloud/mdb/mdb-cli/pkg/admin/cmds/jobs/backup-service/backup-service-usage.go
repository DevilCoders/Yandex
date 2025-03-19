package backupservice

import (
	"context"
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/helpers"
)

const (
	backupUsageCommandName          = "backup-service-usage"
	backupCliBackupUsageCommandName = "backup_service_usage"
	enabledFlag                     = "enabled"
)

var (
	usageCmd = initUsageCommand()

	enabled = false
)

func initUsageCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   fmt.Sprintf("%s --%s=<true|false>", backupUsageCommandName, enabledFlag),
		Short: "Set backup service usage.",
		Args:  cobra.NoArgs,
	}

	cmd.Flags().BoolVar(&enabled, enabledFlag, true, "backup service should be enabled")

	return &cli.Command{Cmd: cmd, Run: runEnableBackupUsage}
}

func runEnableBackupUsage(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	backupUsageArgs := []string{
		fmt.Sprintf("--%s", clusterIDFlag),
		clusterID,
		fmt.Sprintf("--%s=%v", enabledFlag, enabled),
	}

	helpers.RunBackupWorkerJob(env, backupCliBackupUsageCommandName, backupUsageArgs)
}
