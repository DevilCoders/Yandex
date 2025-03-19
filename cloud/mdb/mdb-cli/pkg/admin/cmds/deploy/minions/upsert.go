package minions

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
)

var (
	cmdUpsert = initUpsert()
)

func initUpsert() *cli.Command {
	cmd := &cobra.Command{
		Use:   "upsert <list of fqdns and/or conductor groups in executer format>",
		Short: "Create new salt minions or update the old ones",
		Long:  "Create new salt minions or update the old ones.",
		Args:  cobra.ExactArgs(1),
	}

	cmd.Flags().StringVar(
		&flagGroup,
		flagNameGroup,
		"",
		"Group this minion belongs to",
	)

	cmd.Flags().BoolVar(
		&flagAutoReassign,
		flagNameAutoReassign,
		true,
		"Determines if minion should be automatically reassigned to new master if current one fails",
	)

	cmd.Flags().StringVar(
		&flagMaster,
		flagNameMaster,
		"",
		"Master this minion belongs to",
	)

	return &cli.Command{Cmd: cmd, Run: Upsert}
}

// Upsert salt minion
func Upsert(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	targets := helpers.ParseMultiTargets(ctx, args, env.Logger)
	targets = helpers.ManagedFQDNsToUnamanged(targets, cfg.DeploymentConfig(), env.Logger)
	dapi := helpers.NewDeployAPI(env)

	attrs := deployapi.UpsertMinionAttrs{}
	if cmd.Flag(flagNameGroup).Changed {
		attrs.Group.Set(flagGroup)
	}
	if cmd.Flag(flagNameAutoReassign).Changed {
		attrs.AutoReassign.Set(flagAutoReassign)
	}
	if cmd.Flag(flagNameMaster).Changed {
		attrs.Master.Set(flagMaster)
	}

	env.Logger.Infof("Upserting minions: %s", targets)

	res := helpers.RunMultiTarget(
		ctx,
		targets,
		func(target string) (bool, error) {
			_, err := dapi.UpsertMinion(ctx, target, attrs)
			if err != nil {
				env.Logger.Errorf("Failed to upsert minion: %s", err)
				return false, err
			}

			env.Logger.Debugf("Upserted minion %q", target)
			return true, nil
		},
	)

	helpers.OutputMultiTargetResult(env, res)
}
