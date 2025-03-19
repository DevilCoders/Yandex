package cmd

import (
	"context"
	"fmt"
	"time"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal"
	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	monrunrunner "a.yandex-team.ru/cloud/mdb/internal/monrun/runner"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	warnSince time.Duration
	critSince time.Duration
)

func agePorto(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	releaser := internal.NewApp(conf, env.App)
	age, err := releaser.AgePorto(ctx, portoImageName)
	monrunAge(age, warnSince, critSince, err)
}

func ageCompute(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	releaser := internal.NewApp(conf, env.App)
	age, err := releaser.AgeCompute(ctx, computeImageName, computeFolderID)
	monrunAge(age, warnSince, critSince, err)
}

func monrunAge(age, warnSince, critSince time.Duration, err error) {
	monrunrunner.RunCheck(func(ctx context.Context, logger log.Logger) monrun.Result {
		if err != nil && !xerrors.Is(err, images.ErrLastNotFound) {
			return monrun.Result{Code: monrun.WARN, Message: err.Error()}
		}
		msg := fmt.Sprintf("image is %v old", age)
		switch {
		case xerrors.Is(err, images.ErrLastNotFound):
			return monrun.Result{Code: monrun.CRIT, Message: "no image"}
		case age < warnSince:
			return monrun.Result{Code: monrun.OK, Message: msg}
		case age < critSince:
			return monrun.Result{Code: monrun.WARN, Message: msg}
		default:
			return monrun.Result{Code: monrun.CRIT, Message: msg}
		}
	})
}

func addFlags(cmd *cobra.Command, toAdd *pflag.FlagSet, required []string) {
	cmd.PersistentFlags().AddFlagSet(toAdd)
	for _, flag := range required {
		err := cmd.MarkPersistentFlagRequired(flag)
		if err != nil {
			panic(err)
		}
	}
}
