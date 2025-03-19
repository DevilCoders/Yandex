package minions

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
)

var (
	cmdGet = initGet()
)

func initGet() *cli.Command {
	cmd := &cobra.Command{
		Use:   "get <fqdn>",
		Short: "Get salt minion",
		Long:  "Get salt minion.",
		Args:  cobra.ExactArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Get}
}

// Get salt minion
func Get(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	targets := helpers.ManagedFQDNsToUnamanged([]string{args[0]}, cfg.DeploymentConfig(), env.Logger)
	dapi := helpers.NewDeployAPI(env)

	minion, err := dapi.GetMinion(ctx, targets[0])
	if err != nil {
		env.Logger.Fatalf("Failed to load minion: %s", err)
	}

	var m []byte
	m, err = env.OutMarshaller.Marshal(minion)
	if err != nil {
		env.Logger.Errorf("Failed to marshal minion '%+v': %s", minion, err)
	}

	env.Logger.Info(string(m))
}
