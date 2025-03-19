package masters

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

var (
	Cmd = initCommand()
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "masters",
		Short: "Health actions for masters",
		Long:  "Perform health actions for masters.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd:         cmd,
		SubCommands: []*cli.Command{mastersGet},
	}
}
