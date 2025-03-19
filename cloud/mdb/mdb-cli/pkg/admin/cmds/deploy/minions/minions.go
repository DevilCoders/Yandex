package minions

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

var (
	// Cmd ...
	Cmd = initCommand()

	flagGroup  string
	flagMaster string
)

const (
	flagNameGroup  = "group"
	flagNameMaster = "master"
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "minions",
		Short: "Deploy actions for minions",
		Long:  "Perform salt deployment actions for minions.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd:         cmd,
		SubCommands: []*cli.Command{cmdCreate, cmdUpsert, cmdGet, cmdList, cmdUnregister, cmdDelete},
	}
}
