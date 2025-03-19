package danger

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

var (
	Cmd = initCommand()
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "danger",
		Short: "Danger CIDs when DC is gone",
		Long:  "Perform health actions for danger CIDs.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd:         cmd,
		SubCommands: []*cli.Command{dangerList},
	}
}
