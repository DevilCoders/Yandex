package commands

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
)

var (
	cmdGet = initGet()
)

func initGet() *cli.Command {
	cmd := &cobra.Command{
		Use:   "get <name>",
		Short: "Get command",
		Long:  "Get command.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Get}
}

// Get command
func Get(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	id, err := models.ParseCommandID(args[0])
	if err != nil {
		env.Logger.Fatalf("Invalid command id type: %s", err)
	}

	command, err := dapi.GetCommand(ctx, id)
	if err != nil {
		env.Logger.Fatalf("Failed to load deploy command: %s", err)
	}

	g, err := env.OutMarshaller.Marshal(command)
	if err != nil {
		env.Logger.Errorf("Failed to marshal deploy command '%+v': %s", command, err)
	}

	env.Logger.Info(string(g))
}
