package jobresults

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
		Use:   "jobresults",
		Short: "Deploy actions for job results",
		Long:  "Perform salt deployment actions for job results.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd:         cmd,
		SubCommands: []*cli.Command{cmdGet, cmdList},
	}
}
