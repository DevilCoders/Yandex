package groups

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
		Short: "List groups",
		Long:  "List all groups.",
		Args:  cobra.MinimumNArgs(0),
	}

	paging.Register(cmd.Flags())
	return &cli.Command{Cmd: cmd, Run: List}
}

// List job results
func List(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	groups, page, err := dapi.GetGroups(ctx, paging.Paging())
	if err != nil {
		env.Logger.Fatalf("Failed to load groups: %s", err)
	}

	for _, group := range groups {
		var s []byte
		s, err = env.OutMarshaller.Marshal(group)
		if err != nil {
			env.Logger.Errorf("Failed to marshal group '%+v': %s", group, err)
		}

		env.Logger.Info(string(s))
	}

	p, err := env.OutMarshaller.Marshal(page)
	if err != nil {
		env.Logger.Fatalf("Failed to marshal paging '%+v': %s", page, err)
	}

	env.Logger.Info(string(p))
}
