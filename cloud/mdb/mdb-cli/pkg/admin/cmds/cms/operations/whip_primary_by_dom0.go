package operations

import (
	"context"
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/cms/helpers"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdWhipPrimaryByDom0 = initWhipPrimaryByDom0()
)

func initWhipPrimaryByDom0() *cli.Command {
	cmd := &cobra.Command{
		Use:   "whip_primary_by_dom0 <list of dom0>",
		Short: "Whip primary from instances by dom0",
		Long:  "Whip primary from instances by dom0.",
		Args:  cobra.MinimumNArgs(1),
	}

	cmd.Flags().StringVarP(
		&comment,
		flagNameComment,
		"c",
		"",
		"Comment about operation",
	)

	return &cli.Command{Cmd: cmd, Run: WhipPrimaryByDom0}
}

func WhipPrimaryByDom0(ctx context.Context, env *cli.Env, _ *cobra.Command, args []string) {

	api := helpers.NewCMSClient(ctx, env)

	ctx = helpers.CtxWithNewIdempotence(ctx, env)
	instancesResponces, err := api.ResolveInstancesByDom0(ctx, args)
	if err != nil {
		env.L().Fatal("resolve instances", log.Error(err))
	}

	for _, instance := range instancesResponces.Instance {

		ctx = helpers.CtxWithNewIdempotence(ctx, env)
		instanceFullName := fmt.Sprintf("%s:%s", instance.Dom0, instance.Fqdn)
		op, err := api.CreateWhipPrimaryOperation(ctx, instanceFullName, comment)
		if err != nil {
			env.L().Warn("create operation on instance", log.String("instance_name", instanceFullName), log.Error(err))
		} else {
			helpers.OutputOperation(env, op)
		}
	}
}
