package fixextdns

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/helpers"
)

var (
	Cmd = initCommand()
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "fix-ext-dns [-h] [-c CONFIG [CONFIG ...]] [hosts ...]",
		Short: "Fix external DNS for list of FQDNs.",
		Long:  "Fix external DNS for list of FQDNs.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Create}
}

func Create(_ context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	args = append([]string{"--"}, args...)
	helpers.RunWorkerJob(env, "fix-ext-dns", args)
}
