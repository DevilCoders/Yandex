package certhost

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
		Use:   "cert-host [-h] [-c CONFIG [CONFIG ...]] fqdn [alt_names ...]",
		Short: "Issue certificated for target host and alt_names.",
		Long:  "Issue certificated for target host and alt_names.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Create}
}

func Create(_ context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	args = append([]string{"--"}, args...)
	helpers.RunWorkerJob(env, "cert-host", args)
}
