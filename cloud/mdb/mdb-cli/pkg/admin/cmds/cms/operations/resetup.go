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
	cmdResetup = initResetup()

	comment string
	force   bool
)

const (
	flagNameComment = "comment"
	flagNameForce   = "force"
)

func initResetup() *cli.Command {
	cmd := &cobra.Command{
		Use:   "resetup --force --comment <ticket_id> <instance_id>",
		Short: "Resetup instance",
		Long:  "Resetup instance.",
		Args:  cobra.MinimumNArgs(1),
	}

	cmd.Flags().BoolVarP(
		&force,
		flagNameForce,
		"f",
		false,
		"Force resetup",
	)

	cmd.Flags().StringVarP(
		&comment,
		flagNameComment,
		"c",
		"",
		"Comment about resetup",
	)

	_ = cobra.MarkFlagRequired(cmd.Flags(), flagNameComment)

	return &cli.Command{Cmd: cmd, Run: Resetup}
}

// Resetup instance operations
func Resetup(ctx context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	api := helpers.NewCMSClient(ctx, env)
	instanceID := args[0]

	idem, err := idempotence.New()
	if err != nil {
		env.L().Fatal("can not create idempotence", log.Error(err))
	}
	ctx = idempotence.WithOutgoing(ctx, idem)

	op, err := api.CreateMoveInstanceOperation(ctx, instanceID, comment, force)
	if err != nil {
		env.L().Fatal("can not create operation", log.Error(err))
	}

	helpers.OutputOperation(env, op)
}
