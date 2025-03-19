package cli

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/cli/cmds"
)

func New(name string) *cobra.Command {
	rootCmd := &cli.Command{
		Cmd: &cobra.Command{
			Use: name,
		},
		SubCommands: []*cli.Command{cmds.ValidateClustersCmd},
		Run: func(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
			_ = cmd.Help()
		},
	}
	env := cli.NewWithOptions(context.Background(), rootCmd, cli.WithCustomConfig(defaultConfigPath))

	cobra.OnInitialize(func() {
		conf, err := LoadConfig(env.GetConfigPath(), env.L())
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		env.Config = &conf
	})

	return env.RootCmd.Cmd
}
