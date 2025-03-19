package cmds

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	intapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app/mdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/cli/logic"
)

var (
	ValidateClustersCmd = initValidateClusters()

	flagClusterType string
)

const (
	flagNameClusterType = "type"
)

func initValidateClusters() *cli.Command {
	cmd := &cobra.Command{
		Use:   "validate-clusters",
		Short: "Validate all clusters",
		Long:  "Calls Validate() on pillar of each cluster of specified type.",
		Args:  cobra.MinimumNArgs(0),
	}

	cmd.Flags().StringVarP(
		&flagClusterType,
		flagNameClusterType,
		"t",
		"",
		"Type of cluster",
	)

	if err := cmd.MarkFlagRequired(flagNameClusterType); err != nil {
		panic(err)
	}

	return &cli.Command{Cmd: cmd, Run: validateClusters}
}

func validateClusters(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	err := logic.ValidateClusters(ctx, *env.Config.(*intapi.Config), env.L(), flagClusterType)
	if err != nil {
		env.L().Error(err.Error())
	}
}
