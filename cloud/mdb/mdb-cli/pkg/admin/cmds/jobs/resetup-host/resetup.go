package resetuphost

import (
	"context"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/helpers"
)

const (
	flagNameMethod   = "method"
	flagNameSave     = "save"
	flagNameOffline  = "offline"
	flagNamePreserve = "preserve"
	flagNameIgnore   = "ignore"
)

var (
	Cmd      = initCommand()
	method   string
	save     bool
	offline  bool
	preserve bool
	ignore   []string

	methodFlag *pflag.Flag
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "resetup-host [-m [{readd,restore}]] [-s] [-o] [-p] [-i [IGNORE ...]] host",
		Short: "Run resetup-host job.",
		Long:  "Run resetup-host job.",
		Args:  cobra.ExactArgs(1),
	}

	cmd.Flags().StringVarP(
		&method,
		flagNameMethod,
		"m",
		"readd",
		"Resetup method (readd or restore, default: readd)",
	)
	_ = cmd.RegisterFlagCompletionFunc(flagNameMethod, func(cmd *cobra.Command, args []string, toComplete string) ([]string, cobra.ShellCompDirective) {
		return []string{"readd", "restore"}, cobra.ShellCompDirectiveDefault
	})
	methodFlag = cmd.Flags().Lookup(flagNameMethod)

	cmd.Flags().BoolVarP(
		&save,
		flagNameSave,
		"s",
		false,
		"Save instance disks (if possible)",
	)

	cmd.Flags().BoolVarP(
		&offline,
		flagNameOffline,
		"o",
		false,
		"Operate on stopped cluster",
	)

	cmd.Flags().BoolVarP(
		&preserve,
		flagNamePreserve,
		"p",
		false,
		"Skip destructive actions (in porto)",
	)

	cmd.Flags().StringSliceVarP(
		&ignore,
		flagNameIgnore,
		"i",
		nil,
		"Drop FQDNs from task args",
	)

	return &cli.Command{Cmd: cmd, Run: Create}
}

func Create(_ context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	resetupArgs := make([]string, 0)

	if methodFlag != nil && methodFlag.Changed {
		resetupArgs = append(resetupArgs, "-m", method)
	}

	if save {
		resetupArgs = append(resetupArgs, "-s")
	}

	if offline {
		resetupArgs = append(resetupArgs, "-o")
	}

	if preserve {
		resetupArgs = append(resetupArgs, "-p")
	}

	if ignore != nil {
		resetupArgs = append(resetupArgs, "-i")
		resetupArgs = append(resetupArgs, ignore...)
	}

	resetupArgs = append(resetupArgs, "--", args[0])

	helpers.RunWorkerJob(env, "resetup-host", resetupArgs)
}
