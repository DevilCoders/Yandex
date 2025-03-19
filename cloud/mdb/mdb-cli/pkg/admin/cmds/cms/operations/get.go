package operations

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/cms/helpers"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdGet = initGet()
)

func initGet() *cli.Command {
	cmd := &cobra.Command{
		Use:   "get <operation_id>",
		Short: "Get operation",
		Long:  "Get instance operation.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Get}
}

// Get instance operation
func Get(ctx context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	api := helpers.NewCMSClient(ctx, env)
	operationID := args[0]

	op, err := api.GetInstanceOperation(ctx, operationID)
	if err != nil {
		env.L().Fatal("can not get operation", log.Error(err))
	}

	helpers.OutputOperation(env, op)
}
