package masters

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
)

var (
	cmdCreate = initCreate()

	flagAliases     []string
	flagGroup       string
	flagIsOpen      bool
	flagDescription string
)

const (
	flagNameAliases     = "aliases"
	flagNameGroup       = "group"
	flagNameIsOpen      = "isopen"
	flagNameDescription = "desc"
)

func initCreate() *cli.Command {
	cmd := &cobra.Command{
		Use:   "create <fqdn>",
		Short: "Create salt master",
		Long:  "Create salt master.",
		Args:  cobra.ExactArgs(1),
	}

	cmd.Flags().StringSliceVar(
		&flagAliases,
		flagNameAliases,
		nil,
		"Master's aliases",
	)

	cmd.Flags().StringVar(
		&flagGroup,
		flagNameGroup,
		"",
		"Group this master belongs to",
	)
	if err := cmd.MarkFlagRequired(flagNameGroup); err != nil {
		panic(err)
	}

	cmd.Flags().BoolVar(
		&flagIsOpen,
		flagNameIsOpen,
		false,
		"Determines if master open to new minions",
	)
	if err := cmd.MarkFlagRequired(flagNameIsOpen); err != nil {
		panic(err)
	}

	cmd.Flags().StringVar(
		&flagDescription,
		flagNameDescription,
		"",
		"Master's Description",
	)

	return &cli.Command{Cmd: cmd, Run: Create}
}

// Create salt master
func Create(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	master, err := dapi.CreateMaster(ctx, args[0], flagGroup, flagIsOpen, flagDescription)
	if err != nil {
		env.Logger.Fatalf("Failed to create master: %s", err)
	}

	m, err := env.OutMarshaller.Marshal(master)
	if err != nil {
		env.Logger.Errorf("Failed to marshal master '%+v': %s", master, err)
	}

	env.Logger.Info(string(m))
}
