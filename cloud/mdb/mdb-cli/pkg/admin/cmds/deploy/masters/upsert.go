package masters

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
)

var (
	cmdUpsert = initUpsert()
)

func initUpsert() *cli.Command {
	cmd := &cobra.Command{
		Use:   "upsert <fqdn>",
		Short: "Create new salt master or update the old one",
		Long:  "Create new salt master or update the old one.",
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

	cmd.Flags().BoolVar(
		&flagIsOpen,
		flagNameIsOpen,
		false,
		"Determines if master open to new minions (default false)",
	)

	cmd.Flags().StringVar(
		&flagDescription,
		flagNameDescription,
		"",
		"Master's Description",
	)

	return &cli.Command{Cmd: cmd, Run: Upsert}
}

// Upsert new salt master or update the old one
func Upsert(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	attrs := deployapi.UpsertMasterAttrs{}
	if cmd.Flag(flagNameGroup).Changed {
		attrs.Group.Set(flagGroup)
	}
	if cmd.Flag(flagNameIsOpen).Changed {
		attrs.IsOpen.Set(flagIsOpen)
	}
	if cmd.Flag(flagNameDescription).Changed {
		attrs.Description.Set(flagDescription)
	}

	master, err := dapi.UpsertMaster(ctx, args[0], attrs)
	if err != nil {
		env.Logger.Fatalf("Failed to upsert master: %s", err)
	}

	m, err := env.OutMarshaller.Marshal(master)
	if err != nil {
		env.Logger.Errorf("Failed to marshal master '%+v': %s", master, err)
	}

	env.Logger.Info(string(m))
}
