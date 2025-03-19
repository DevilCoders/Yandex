package cmd

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

const configName = "mdb-image-releaser.yaml"

var (
	rootCmd = &cli.Command{
		Cmd: &cobra.Command{
			Use:   "mdb-image-releaser",
			Short: "Releases images and checks their age",
		},
		SubCommands: []*cli.Command{computeSubCommand, portoSubCommand},
		Run:         runRoot,
	}

	conf = internal.DefaultConfig()
)

func runRoot(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {

}

func Execute(ctx context.Context) {
	env := cli.NewWithOptions(ctx, rootCmd, cli.WithFlagLogShowAll(), cli.WithConfigLoad(&conf, configName), cli.WithExtraAppOptions(app.WithSentry()))
	err := env.RootCmd.Cmd.ExecuteContext(ctx)
	if err != nil {
		env.L().Errorf("%s", err)
	}
}
