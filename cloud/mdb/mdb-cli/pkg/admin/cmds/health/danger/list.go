package danger

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/health/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	dangerList = initList()
)

func initList() *cli.Command {
	cmd := &cobra.Command{
		Use:   "list <DC>",
		Short: "List danger CIDs",
		Long:  "List CIDs in danger situation.",
		Args:  cobra.ExactArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: List}
}

func List(ctx context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	c := helpers.NewConductorClient(env)
	hosts, err := c.GroupToHosts(ctx, "mdb_dataplane_porto_prod", conductor.GroupToHostsAttrs{DC: optional.NewString(args[0])})
	if err != nil {
		env.L().Fatalf("can not get hosts from conductor: %s", err)
	}

	api := helpers.NewHealthClient(env)
	danger := make(map[string]types.HostNeighboursInfo)
	for i := 0; i < len(hosts); i += 100 {
		var r int
		if i+100 < len(hosts) {
			r = i + 100
		} else {
			r = len(hosts) - 1
		}
		toDanger(ctx, env, api, hosts[i:r], danger)
	}

	if len(danger) > 0 {
		helpers.OutputDanger(env, danger)
	} else {
		env.L().Info("nothing danger")
	}
}

func toDanger(ctx context.Context, env *cli.Env, api client.MDBHealthClient, hosts []string, res map[string]types.HostNeighboursInfo) {
	neigh, err := api.GetHostNeighboursInfo(ctx, hosts)
	if err != nil {
		env.L().Fatalf("can not get neighbours: %s", err)
	}

	hostshealth, err := api.GetHostsHealth(ctx, hosts)
	if err != nil {
		env.L().Fatalf("can not get host health: %s", err)
	}

	for _, h := range hostshealth {
		ni, ok := neigh[h.FQDN()]
		if !ok {
			env.L().Warn("can't find in neighbours info", log.String("fqdn", h.FQDN()))
			continue
		}
		if !ni.IsHA() || ni.SameRolesTotal == ni.SameRolesAlive {
			continue
		}
		res[h.FQDN()] = ni
	}
}
