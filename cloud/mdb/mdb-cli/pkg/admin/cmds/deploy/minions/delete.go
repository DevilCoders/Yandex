package minions

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
)

var (
	cmdDelete = initDelete()
)

func initDelete() *cli.Command {
	cmd := &cobra.Command{
		Use:   "delete <list of fqdns and/or conductor groups in executer format>",
		Short: "Delete salt minion",
		Long:  "Delete salt minion.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Delete}
}

// Delete salt minion
func Delete(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	targets := helpers.ParseMultiTargets(ctx, args, env.Logger)
	targets = helpers.ManagedFQDNsToUnamanged(targets, cfg.DeploymentConfig(), env.Logger)
	dapi := helpers.NewDeployAPI(env)

	env.Logger.Infof("Deleting minions: %s", targets)

	res := helpers.RunMultiTarget(
		ctx,
		targets,
		func(target string) (bool, error) {
			if env.IsDryRunMode() {
				env.Logger.Debugf("Would have deleted minion %q, but we are in the dry run", target)
				return true, nil
			}

			// Delete
			if err := dapi.DeleteMinion(ctx, target); err != nil {
				env.Logger.Errorf("failed to delete minion %q: %s", target, err)
				return false, err
			}

			env.Logger.Debugf("Deleted minion %q", target)
			return true, nil
		},
	)

	helpers.OutputMultiTargetResult(env, res)
}
