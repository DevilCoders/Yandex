package cmds

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	defaultConfigPath = "/etc/mdb_porto_agent.yaml"
)

// New constructs cli for mdb-porto-agent tool
func New(cmdName string) (*cli.Env, error) {
	// Construct and configure root command
	cc := &cli.Command{
		Cmd: &cobra.Command{
			Use: cmdName,
		},
		SubCommands: []*cli.Command{cmdUpdate, cmdWakeup, cmdClean},
		Run:         run,
	}
	env := cli.NewWithOptions(context.Background(), cc,
		cli.WithFlagLogShowAll(),
		cli.WithFlagDryRun(),
		cli.WithCustomConfig(defaultConfigPath))
	logger, err := zap.New(zap.CLIConfig(log.InfoLevel))
	if err != nil {
		return nil, err
	}
	env.Logger = logger
	conf, err := config.LoadConfig(env.GetConfigPath(), env.Logger)
	if err != nil {
		return nil, err
	}
	env.Config = &conf
	env.Logger, err = zap.New(zap.CLIConfig(conf.LogLevel))
	if err != nil {
		return nil, err
	}
	return env, nil
}

func run(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	_ = cmd.Help()
}
