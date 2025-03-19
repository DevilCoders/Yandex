package masters

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
		Use:   "masters",
		Short: "Deploy actions for masters",
		Long:  "Perform salt deployment actions for masters.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd:         cmd,
		SubCommands: []*cli.Command{cmdCreate, cmdUpsert, cmdGet, cmdList, cmdBalance},
	}
}
