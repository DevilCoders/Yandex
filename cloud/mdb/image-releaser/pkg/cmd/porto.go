package cmd

import (
	"context"
	"time"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

var (
	portoSubCommand = &cli.Command{
		Cmd: &cobra.Command{
			Use: "porto",
		},
		SubCommands: []*cli.Command{portoReleaseSubCommand, portoAgeSubCommand},
	}
	portoReleaseSubCommand = &cli.Command{
		Cmd: &cobra.Command{
			Short: "Creates or releases images in porto",
			Use:   "release",
		},
		Run: releasePorto,
	}
	portoAgeSubCommand = &cli.Command{
		Cmd: &cobra.Command{
			Use:   "age",
			Short: "Checks image age",
		},
		Run: agePorto,
	}

	//flags
	portoReqStability                 time.Duration
	portoImageName                    string
	portoCheckHost, portoCheckService string
	portoNoCheck                      bool
)

func init() {
	portoCommonFlags := pflag.NewFlagSet("porto", pflag.ContinueOnError)
	portoCommonFlags.StringVar(&portoImageName, "name", "", "Image name")

	//release
	portoReleaseSubCommand.Cmd.PersistentFlags().DurationVar(&portoReqStability, "stable", time.Hour*4, "Required stability duration")
	portoReleaseSubCommand.Cmd.PersistentFlags().StringVar(&portoCheckHost, "check-host", "", "Juggler check hostname")
	portoReleaseSubCommand.Cmd.PersistentFlags().StringVar(&portoCheckService, "check-service", "", "Juggler check service")
	portoReleaseSubCommand.Cmd.PersistentFlags().BoolVar(&portoNoCheck, "no-check", false, "Release without checking stability")
	addFlags(portoReleaseSubCommand.Cmd, portoCommonFlags, []string{"name"})
	//age
	portoAgeSubCommand.Cmd.PersistentFlags().DurationVar(&warnSince, "warn", time.Hour*25, "Warn level")
	portoAgeSubCommand.Cmd.PersistentFlags().DurationVar(&critSince, "crit", time.Hour*50, "Crit level")
	addFlags(portoAgeSubCommand.Cmd, portoCommonFlags, []string{"name"})
}

func releasePorto(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	releaser := internal.NewApp(conf, env.App)
	releaser.ReleasePorto(ctx, portoImageName, portoNoCheck, portoCheckHost, portoCheckService, portoReqStability)
}
