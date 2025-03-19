package replacerootfs

import (
	"context"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/helpers"
)

const (
	flagNameVtype        = "vtype"
	flagNameTLS          = "tls"
	flagNameOffline      = "offline"
	flagNamePreserve     = "preserve"
	flagNameArgs         = "args"
	flagNameImage        = "image"
	flagNameBootstrapCmd = "bootstrap_cmd"
	flagNameSkipDNS      = "skip_dns"
)

var (
	Cmd = initCommand()

	vtype        string
	tls          bool
	offline      bool
	preserve     bool
	hsArgs       string
	image        string
	bootstrapCmd string
	skipDNS      bool

	vtypeFlag *pflag.Flag
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "replace-rootfs [-h] [-c CONFIG [CONFIG ...]] [-a ARGS] [-v [{compute,porto}]] [-o] [-t] [-i IMAGE] [-b BOOTSTRAP_CMD] [-p] [-s] host",
		Short: "Run replace-rootfs job.",
		Long:  "Run replace-rootfs job.",
		Args:  cobra.ExactArgs(1),
	}

	cmd.Flags().StringVarP(
		&vtype,
		flagNameVtype,
		"v",
		"compute",
		"Target vtype (compute or porto, default: compute)",
	)
	_ = cmd.RegisterFlagCompletionFunc(flagNameVtype, func(cmd *cobra.Command, args []string, toComplete string) ([]string, cobra.ShellCompDirective) {
		return []string{"compute", "porto"}, cobra.ShellCompDirectiveDefault
	})
	vtypeFlag = cmd.Flags().Lookup(flagNameVtype)

	cmd.Flags().BoolVarP(
		&tls,
		flagNameTLS,
		"t",
		false,
		"Update TLS cert",
	)

	cmd.Flags().BoolVarP(
		&offline,
		flagNameOffline,
		"o",
		false,
		"Do offline replace (compute only)",
	)

	cmd.Flags().BoolVarP(
		&preserve,
		flagNamePreserve,
		"p",
		false,
		"Preserve original disk size",
	)

	cmd.Flags().BoolVarP(
		&skipDNS,
		flagNameSkipDNS,
		"s",
		false,
		"Do not touch dns",
	)

	cmd.Flags().StringVarP(
		&hsArgs,
		flagNameArgs,
		"a",
		"",
		"Pass additional args to highstate call",
	)

	cmd.Flags().StringVarP(
		&image,
		flagNameImage,
		"i",
		"",
		"Image type override",
	)

	cmd.Flags().StringVarP(
		&bootstrapCmd,
		flagNameBootstrapCmd,
		"b",
		"",
		"DBM bootstrap command",
	)

	return &cli.Command{Cmd: cmd, Run: Create}
}

func Create(_ context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	rrArgs := make([]string, 0)

	if vtypeFlag != nil && vtypeFlag.Changed {
		rrArgs = append(rrArgs, "-v", vtype)
	}

	if hsArgs != "" {
		rrArgs = append(rrArgs, "-a", hsArgs)
	}

	if offline {
		rrArgs = append(rrArgs, "-o")
	}

	if preserve {
		rrArgs = append(rrArgs, "-p")
	}

	if tls {
		rrArgs = append(rrArgs, "-t")
	}

	if image != "" {
		rrArgs = append(rrArgs, "-i", image)
	}

	if bootstrapCmd != "" {
		rrArgs = append(rrArgs, "-b", bootstrapCmd)
	}

	if skipDNS {
		rrArgs = append(rrArgs, "-s")
	}

	rrArgs = append(rrArgs, "--", args[0])

	helpers.RunWorkerJob(env, "replace-rootfs", rrArgs)
}
