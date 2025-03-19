package ops

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/ops/services"
)

var (
	// Cmd ...
	Cmd = initCommand()
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "ops",
		Short: "Operations actions",
		Long:  "Perform DevOps actions.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd:         cmd,
		SubCommands: []*cli.Command{services.Cmd},
	}
}
