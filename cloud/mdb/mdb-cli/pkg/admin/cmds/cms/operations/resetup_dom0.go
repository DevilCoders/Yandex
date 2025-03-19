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
	cmdResetupDom0 = initResetupDom0()
)

func initResetupDom0() *cli.Command {
	cmd := &cobra.Command{
		Use:   "resetup_dom0 --comment <ticket_id> <dom0> <dom0> ...",
		Short: "Resetup all instances from dom0",
		Long:  "Resetup all instances from dom0.",
		Args:  cobra.MinimumNArgs(1),
	}

	cmd.Flags().StringVarP(
		&comment,
		flagNameComment,
		"c",
		"",
		"Comment about resetup",
	)

	_ = cobra.MarkFlagRequired(cmd.Flags(), flagNameComment)

	return &cli.Command{Cmd: cmd, Run: ResetupDom0}
}

// ResetupDom0 instance operations
func ResetupDom0(ctx context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	api := helpers.NewCMSClient(ctx, env)

	ctx = helpers.CtxWithNewIdempotence(ctx, env)
	instancesResponces, err := api.ResolveInstancesByDom0(ctx, args)
	if err != nil {
		env.L().Fatal("resolve instances", log.Error(err))
	}
	for _, instance := range instancesResponces.Instance {
		instanceID := fmt.Sprintf("%s:%s", instance.Dom0, instance.Fqdn)
		ctx = helpers.CtxWithNewIdempotence(ctx, env)
		op, err := api.CreateMoveInstanceOperation(ctx, instanceID, comment, false)
		if err != nil {
			env.L().Fatal("can not create operation", log.Error(err))
		}

		helpers.OutputOperation(env, op)
	}
}
