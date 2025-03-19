package operations

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

var (
	// Cmd ...
	Cmd = initCommand()
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "operations",
		Short: "CMS actions for operations",
		Long:  "Perform cms actions for operations.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd:         cmd,
		SubCommands: []*cli.Command{cmdChangeStatus, cmdGet, cmdResetup, cmdResetupDom0, cmdWhipPrimary, cmdWhipPrimaryByDom0},
	}
}
