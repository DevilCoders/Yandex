package groups

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
)

var (
	cmdCreate = initCreate()
)

func initCreate() *cli.Command {
	cmd := &cobra.Command{
		Use:   "create <name>",
		Short: "Create deploy group",
		Long:  "Create deploy group.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Create}
}

// Create deploy group
func Create(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	group, err := dapi.CreateGroup(ctx, args[0])
	if err != nil {
		env.Logger.Fatalf("Failed to create deploy group: %s", err)
	}

	g, err := env.OutMarshaller.Marshal(group)
	if err != nil {
		env.Logger.Errorf("Failed to marshal deploy group '%+v': %s", group, err)
	}

	env.Logger.Info(string(g))
}
