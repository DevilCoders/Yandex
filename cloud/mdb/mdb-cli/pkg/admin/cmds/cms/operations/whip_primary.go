package operations

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/cms/helpers"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdWhipPrimary = initWhipPrimary()
)

func initWhipPrimary() *cli.Command {
	cmd := &cobra.Command{
		Use:   "whip_primary <instance_id>",
		Short: "Whip primary from instance",
		Long:  "Whip primary from instance.",
		Args:  cobra.MinimumNArgs(1),
	}

	cmd.Flags().StringVarP(
		&comment,
		flagNameComment,
		"c",
		"",
		"Comment about operation",
	)

	return &cli.Command{Cmd: cmd, Run: WhipPrimary}
}

func WhipPrimary(ctx context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	api := helpers.NewCMSClient(ctx, env)
	instanceID := args[0]

	idem, err := idempotence.New()
	if err != nil {
		env.L().Fatal("can not create idempotence", log.Error(err))
	}
	ctx = idempotence.WithOutgoing(ctx, idem)

	op, err := api.CreateWhipPrimaryOperation(ctx, instanceID, comment)
	if err != nil {
		env.L().Fatal("can not create operation", log.Error(err))
	}

	helpers.OutputOperation(env, op)
}
