package groups

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
)

var (
	cmdGet = initGet()
)

func initGet() *cli.Command {
	cmd := &cobra.Command{
		Use:   "get <name>",
		Short: "Get deploy group",
		Long:  "Get deploy group.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Get}
}

// Get deploy group
func Get(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	group, err := dapi.GetGroup(ctx, args[0])
	if err != nil {
		env.Logger.Fatalf("Failed to load deploy group: %s", err)
	}

	g, err := env.OutMarshaller.Marshal(group)
	if err != nil {
		env.Logger.Errorf("Failed to marshal deploy group '%+v': %s", group, err)
	}

	env.Logger.Info(string(g))
}
