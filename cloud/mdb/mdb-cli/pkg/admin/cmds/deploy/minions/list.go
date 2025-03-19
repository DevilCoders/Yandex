package minions

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/paging"
)

var (
	cmdList = initList()
)

func initList() *cli.Command {
	cmd := &cobra.Command{
		Use:   "list",
		Short: "List salt minions",
		Long:  "List all known salt minions.",
		Args:  cobra.MinimumNArgs(0),
	}

	paging.Register(cmd.Flags())
	return &cli.Command{Cmd: cmd, Run: List}
}

// List salt minions
func List(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	minions, page, err := dapi.GetMinions(ctx, paging.Paging())
	if err != nil {
		env.Logger.Fatalf("Failed to load minions: %s", err)
	}

	for _, minion := range minions {
		var m []byte
		m, err = env.OutMarshaller.Marshal(minion)
		if err != nil {
			env.Logger.Errorf("Failed to marshal minion '%+v': %s", minion, err)
		}

		env.Logger.Info(string(m))
	}

	p, err := env.OutMarshaller.Marshal(page)
	if err != nil {
		env.Logger.Fatalf("Failed to marshal paging '%+v': %s", page, err)
	}

	env.Logger.Info(string(p))
}
