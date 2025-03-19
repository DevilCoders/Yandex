package backupservice

import (
	"context"
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/helpers"
)

const (
	backupImportCommandName          = "import-backups"
	backupCliBackupImportCommandName = "import_backups"
	dryRunFlag                       = "dry-run"
	skipSchedDateDupsFlag            = "skip-sched-date-dups"
	completeFailedFlag               = "complete-failed"
)

var (
	importCmd = initImportCommand()

	skipSchedDateDups = false
	completeFailed    = false
	dryRun            = true
)

func initImportCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   fmt.Sprintf("%s [--%s, --%s, --%s]", backupImportCommandName, dryRunFlag, skipSchedDateDupsFlag, completeFailedFlag),
		Short: "Imports backups from storage.",
		Args:  cobra.NoArgs,
	}

	cmd.Flags().BoolVar(&dryRun, dryRunFlag, true, "do not insert backups into metadb")
	cmd.Flags().BoolVar(&skipSchedDateDups, "skip-sched-date-dups", false, "skip backup import if violates unique key constraint")
	cmd.Flags().BoolVar(&completeFailed, "complete-failed", false, "complete existing backup if it has failed status")

	return &cli.Command{Cmd: cmd, Run: runImport}
}

func runImport(_ context.Context, env *cli.Env, _ *cobra.Command, _ []string) {
	backupImportArgs := []string{
		fmt.Sprintf("--%s", clusterIDFlag),
		clusterID,
		fmt.Sprintf("--%s=%v", dryRunFlag, dryRun),
		fmt.Sprintf("--%s=%v", skipSchedDateDupsFlag, skipSchedDateDups),
		fmt.Sprintf("--%s=%v", completeFailedFlag, completeFailed),
	}

	helpers.RunBackupWorkerJob(env, backupCliBackupImportCommandName, backupImportArgs)
}
