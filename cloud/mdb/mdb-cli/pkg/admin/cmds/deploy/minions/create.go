package minions

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	cmdCreate = initCreate()

	flagAutoReassign bool
)

const (
	flagNameAutoReassign = "autoreassign"
)

func initCreate() *cli.Command {
	cmd := &cobra.Command{
		Use:   "create <list of fqdns and/or conductor groups in executer format>",
		Short: "Create salt minion",
		Long:  "Create salt minion.",
		Args:  cobra.MinimumNArgs(1),
	}

	cmd.Flags().StringVar(
		&flagGroup,
		flagNameGroup,
		"",
		"Group this minion belongs to",
	)
	if err := cmd.MarkFlagRequired(flagNameGroup); err != nil {
		panic(err)
	}

	cmd.Flags().BoolVar(
		&flagAutoReassign,
		flagNameAutoReassign,
		true,
		"Determines if minion should be automatically reassigned to new master if current one fails",
	)

	return &cli.Command{Cmd: cmd, Run: Create}
}

// Create salt minion(s)
func Create(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	targets := helpers.ParseMultiTargets(ctx, args, env.Logger)
	targets = helpers.ManagedFQDNsToUnamanged(targets, cfg.DeploymentConfig(), env.Logger)
	dapi := helpers.NewDeployAPI(env)

	env.Logger.Infof("Creating minions: %s", targets)

	res := helpers.RunMultiTarget(
		ctx,
		targets,
		func(target string) (bool, error) {
			// Do we have minion already? Do nothing if we found minion
			_, err := dapi.GetMinion(ctx, target)
			if err == nil {
				env.Logger.Debugf("Minion %q is already registered in deploy v2", target)
				return false, nil
			}

			if !xerrors.Is(err, deployapi.ErrNotFound) {
				env.Logger.Errorf("failed to get minion %q: %s", target, err)
				return false, err
			}

			if env.IsDryRunMode() {
				env.Logger.Debugf("Would have registered minion %q in deploy v2, but we are in the dry run", target)
				return true, nil
			}

			if _, err := dapi.CreateMinion(ctx, target, flagGroup, true); err != nil {
				env.Logger.Errorf("failed to create minion %q: %s", target, err)
				return false, err
			}

			env.Logger.Debugf("Registered minion %q in deploy v2", target)
			return true, nil
		},
	)

	helpers.OutputMultiTargetResult(env, res)
}
