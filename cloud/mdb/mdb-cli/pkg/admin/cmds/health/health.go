package health

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/health/danger"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/health/masters"
)

var (
	Cmd = initCommand()
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "health",
		Short: "health actions",
		Long:  "Perform health actions.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd: cmd,
		SubCommands: []*cli.Command{
			masters.Cmd,
			danger.Cmd,
		},
	}
}
