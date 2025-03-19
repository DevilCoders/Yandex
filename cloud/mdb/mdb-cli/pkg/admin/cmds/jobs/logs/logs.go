package logs

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/helpers"
)

var (
	Cmd = initLogs()
)

func initLogs() *cli.Command {
	cmd := &cobra.Command{
		Use:   "logs <name>",
		Short: "List logs",
		Long:  "List job logs.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Logs}
}

func Logs(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	helpers.JobLogs(env, args[0])
}
