package shipments

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
		Use:   "shipments",
		Short: "Deploy actions for shipments",
		Long:  "Perform salt deployment actions for shipments.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd:         cmd,
		SubCommands: []*cli.Command{cmdCreate, cmdGet, cmdList},
	}
}
