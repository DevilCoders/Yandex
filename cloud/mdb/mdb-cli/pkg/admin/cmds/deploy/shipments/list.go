package shipments

import (
	"context"
	"fmt"
	"strings"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/paging"
)

var (
	cmdList = initList()

	flagFQDN           string
	flagShipmentStatus string
)

const (
	flagNameFQDN           = "fqdn"
	flagNameShipmentStatus = "status"
)

func initList() *cli.Command {
	cmd := &cobra.Command{
		Use:   "list",
		Short: "List shipments",
		Long:  "List all shipments.",
		Args:  cobra.MinimumNArgs(0),
	}

	cmd.Flags().StringVar(
		&flagFQDN,
		flagNameFQDN,
		"",
		"Minion fqdn",
	)

	cmd.Flags().StringVar(
		&flagShipmentStatus,
		flagNameShipmentStatus,
		"",
		fmt.Sprintf(
			"Shipment status (valid values are '%s')",
			strings.ToLower(strings.Join(models.ShipmentStatusList(), ",")),
		),
	)

	paging.Register(cmd.Flags())
	return &cli.Command{Cmd: cmd, Run: List}
}

// List shipments
func List(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	attrs := deployapi.SelectShipmentsAttrs{}
	if cmd.Flag(flagNameFQDN).Changed {
		attrs.FQDN.Set(flagFQDN)
	}
	if cmd.Flag(flagNameShipmentStatus).Changed {
		status := models.ParseShipmentStatus(flagShipmentStatus)
		if status == models.ShipmentStatusUnknown {
			env.Logger.Fatalf("invalid shipment status: %s", flagShipmentStatus)
		}

		attrs.Status.Set(string(status))
	}

	shipments, page, err := dapi.GetShipments(ctx, attrs, paging.Paging())
	if err != nil {
		env.Logger.Fatalf("Failed to load shipments: %s", err)
	}

	for _, shipment := range shipments {
		var s []byte
		s, err = env.OutMarshaller.Marshal(shipment)
		if err != nil {
			env.Logger.Errorf("Failed to marshal shipment '%+v': %s", shipment, err)
		}

		env.Logger.Info(string(s))
	}

	p, err := env.OutMarshaller.Marshal(page)
	if err != nil {
		env.Logger.Fatalf("Failed to marshal paging '%+v': %s", page, err)
	}

	env.Logger.Info(string(p))
}
