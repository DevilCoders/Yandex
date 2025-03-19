package app

import (
	"context"
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	Name = "mdb-disklock"
)

var (
	cfg        = DefaultConfig()
	configName = fmt.Sprintf("%s.yaml", Name)
)

func Run(ctx context.Context) {
	rootCmd := &cli.Command{
		Cmd: &cobra.Command{
			Use: Name,
		},
		SubCommands: []*cli.Command{
			cmdClose,
			cmdMount,
			cmdOpen,
			cmdUnmount,
			cmdStart,
			cmdStop,
			cmdFormat,
		},
		Run: func(_ context.Context, _ *cli.Env, cmd *cobra.Command, _ []string) {
			_ = cmd.Help()
		},
	}

	env := cli.NewWithOptions(ctx, rootCmd, cli.WithFlagLogShowAll(), cli.WithCustomConfig(configName))
	if err := env.RootCmd.Cmd.ExecuteContext(ctx); err != nil {
		env.L().Fatal("got error", log.Error(err))
	}
}
