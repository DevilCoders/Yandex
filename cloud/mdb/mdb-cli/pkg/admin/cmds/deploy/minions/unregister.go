package minions

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
)

var (
	cmdUnregister = initUnregister()
)

func initUnregister() *cli.Command {
	cmd := &cobra.Command{
		Use:   "unregister <list of fqdns and/or conductor groups in executer format>",
		Short: "Unregister salt minion",
		Long:  "Unregister salt minion.",
		Args:  cobra.MinimumNArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Unregister}
}

// Unregister salt minion
func Unregister(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	targets := helpers.ParseMultiTargets(ctx, args, env.Logger)
	targets = helpers.ManagedFQDNsToUnamanged(targets, cfg.DeploymentConfig(), env.Logger)
	dapi := helpers.NewDeployAPI(env)

	env.Logger.Infof("Unregistering minions: %s", targets)

	res := helpers.RunMultiTarget(
		ctx,
		targets,
		func(target string) (bool, error) {
			if env.IsDryRunMode() {
				env.Logger.Debugf("Would have unregistered minion %q, but we are in the dry run", target)
				return true, nil
			}

			// Unregister
			if _, err := dapi.UnregisterMinion(ctx, target); err != nil {
				env.Logger.Errorf("failed to unregister minion %q: %s", target, err)
				return false, err
			}

			env.Logger.Debugf("Unregistered minion %q", target)
			return true, nil
		},
	)

	helpers.OutputMultiTargetResult(env, res)
}
