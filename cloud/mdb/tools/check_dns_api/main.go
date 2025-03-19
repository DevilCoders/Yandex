package main

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

const (
	appName           = "check-dns-api"
	defaultConfigPath = "check-dns-api" + ".yaml"
)

func run(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	_ = cmd.Help()
}

func newCli(cmdName string) *cli.Env {
	// Construct and configure root command
	cc := &cli.Command{
		Cmd: &cobra.Command{
			Use: cmdName,
		},
		SubCommands: []*cli.Command{cmdChangeCycles, cmdCleanRecords},
		Run:         run,
	}
	return cli.NewWithOptions(context.Background(), cc,
		cli.WithFlagLogShowAll(),
		cli.WithFlagDryRun(),
		cli.WithCustomConfig(defaultConfigPath))
}

func main() {
	env := newCli(appName)
	if err := env.RootCmd.Cmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
