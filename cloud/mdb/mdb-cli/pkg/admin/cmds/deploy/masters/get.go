package masters

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
		Short: "Get salt master",
		Long:  "Get salt master.",
		Args:  cobra.ExactArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Get}
}

// Get salt master
func Get(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	targets := helpers.ManagedFQDNsToUnamanged([]string{args[0]}, cfg.DeploymentConfig(), env.Logger)
	dapi := helpers.NewDeployAPI(env)

	master, err := dapi.GetMaster(ctx, targets[0])
	if err != nil {
		env.Logger.Fatalf("Failed to load master: %s", err)
	}

	var m []byte
	m, err = env.OutMarshaller.Marshal(master)
	if err != nil {
		env.Logger.Errorf("Failed to marshal master '%+v': %s", master, err)
	}

	env.Logger.Info(string(m))
}
