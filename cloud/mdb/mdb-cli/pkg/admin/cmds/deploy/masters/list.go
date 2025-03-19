package masters

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
		Short: "List salt masters",
		Long:  "List all registers salt masters.",
		Args:  cobra.MinimumNArgs(0),
	}

	paging.Register(cmd.Flags())
	return &cli.Command{Cmd: cmd, Run: List}
}

// List salt masters
func List(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	masters, page, err := dapi.GetMasters(ctx, paging.Paging())
	if err != nil {
		env.Logger.Fatalf("Failed to load masters: %s", err)
	}

	for _, master := range masters {
		var m []byte
		m, err = env.OutMarshaller.Marshal(master)
		if err != nil {
			env.Logger.Errorf("Failed to marshal master '%+v': %s", master, err)
		}

		env.Logger.Info(string(m))
	}

	p, err := env.OutMarshaller.Marshal(page)
	if err != nil {
		env.Logger.Fatalf("Failed to marshal paging '%+v': %s", page, err)
	}

	env.Logger.Info(string(p))
}
