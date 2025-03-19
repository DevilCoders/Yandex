package masters

import (
	"context"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/health/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

var (
	mastersGet = initGet()
)

func initGet() *cli.Command {
	cmd := &cobra.Command{
		Use:   "get <DC>",
		Short: "Get masters",
		Long:  "Get masters.",
		Args:  cobra.ExactArgs(1),
	}

	return &cli.Command{Cmd: cmd, Run: Get}
}

func Get(ctx context.Context, env *cli.Env, _ *cobra.Command, args []string) {
	cndcl := helpers.NewConductorClient(env)
	hosts, err := cndcl.GroupToHosts(ctx, "mdb_dataplane_porto_prod", conductor.GroupToHostsAttrs{DC: optional.NewString(args[0])})
	if err != nil {
		env.L().Fatalf("can not get hosts from conductor: %s", err)
	}

	api := helpers.NewHealthClient(env)

	masters := make(map[string]types.HostNeighboursInfo)
	for i := 0; i < len(hosts); i += 100 {
		var r int
		if i+100 < len(hosts) {
			r = i + 100
		} else {
			r = len(hosts) - 1
		}
		toSwitch(ctx, env, api, hosts[i:r], masters)
	}

	if len(masters) > 0 {
		helpers.OutputMasters(env, masters)
	} else {
		env.L().Info("nothing to switch")
	}
}

func toSwitch(ctx context.Context, env *cli.Env, api client.MDBHealthClient, hosts []string, res map[string]types.HostNeighboursInfo) {
	hostshealth, err := api.GetHostsHealth(ctx, hosts)
	if err != nil {
		env.L().Fatalf("can not get host health: %s", err)
	}

	masters := make([]string, 0)
	for _, hh := range hostshealth {
		for _, sh := range hh.Services() {
			if sh.Role() == types.ServiceRoleMaster {
				masters = append(masters, hh.FQDN())
				break
			}
		}
	}

	if len(masters) == 0 {
		env.L().Debug("there are no masters")
		return
	}

	env.L().Debugf("there are several masters: %v", masters)

	neigh, err := api.GetHostNeighboursInfo(ctx, masters)
	if err != nil {
		env.L().Fatalf("can not get neighbours: %s", err)
	}

	for fqdn, ni := range neigh {
		if ni.IsHA() {
			res[fqdn] = ni
		}
	}
}
