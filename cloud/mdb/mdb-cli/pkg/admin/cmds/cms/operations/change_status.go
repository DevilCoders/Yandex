package operations

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/cms/helpers"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdChangeStatus = initChangeStatus()
	flagStatus      string
)

const (
	flagNameStatus = "status"
)

func initChangeStatus() *cli.Command {
	cmd := &cobra.Command{
		Use:   "change_status --status <status> <operation_id>",
		Short: "Change operation status",
		Long:  "Change instance operation status.",
		Args:  cobra.MinimumNArgs(1),
	}
	cmd.Flags().StringVar(
		&flagStatus,
		flagNameStatus,
		"",
		"Change operation to status",
	)
	_ = cobra.MarkFlagRequired(cmd.Flags(), flagNameStatus)
	_ = cmd.RegisterFlagCompletionFunc(flagNameStatus, func(cmd *cobra.Command, args []string, toComplete string) ([]string, cobra.ShellCompDirective) {
		return []string{string(models.InstanceOperationStatusOK), string(models.InstanceOperationStatusInProgress)}, cobra.ShellCompDirectiveDefault
	})

	return &cli.Command{Cmd: cmd, Run: ChangeStatus}
}

// ChangeStatus instance operation
func ChangeStatus(ctx context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	api := helpers.NewCMSClient(ctx, env)
	operationID := args[0]

	err := api.ChangeOperationStatus(ctx, operationID, models.InstanceOperationStatus(flagStatus))
	if err != nil {
		env.L().Fatal("can not change operation status", log.Error(err))
	}

	env.L().Info("updated")
}
