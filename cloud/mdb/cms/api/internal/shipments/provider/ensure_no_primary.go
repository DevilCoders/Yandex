package provider

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery/metadbdiscovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (p *ShipmentProvider) EnsureNoPrimary(ctx context.Context, fqdn string) (deployModels.ShipmentID, error) {
	ensureFrom, err := p.sameRegionClusterLegsByFQDN(ctx, fqdn)
	if err != nil {
		return 0, xerrors.Errorf("get neighbours from metadb: %w", err)
	}

	cmd := p.ensureFromCMD(ensureFrom)
	shipment, err := p.d.CreateShipment(
		ctx,
		[]string{fqdn},
		cmd,
		int64(1),
		1,
		time.Minute*25,
	)
	if err != nil {
		return 0, err
	}

	return shipment.ID, nil
}

func (p *ShipmentProvider) ensureFromCMD(ensureFrom []string) []deployModels.CommandDef {
	const Timeout = 20
	var cmds = []deployModels.CommandDef{{
		Type: "cmd.run",
		Args: []string{shipments.ShExecIfPresent(
			"/usr/local/yandex/ensure_no_primary.sh",
			fmt.Sprintf("--from-hosts=%s", strings.Join(ensureFrom, ",")),
		)},
		Timeout: encodingutil.FromDuration(time.Minute * time.Duration(Timeout)),
	}}

	return cmds
}

func (p *ShipmentProvider) sameRegionClusterLegsByFQDN(ctx context.Context, fqdn string) ([]string, error) {
	result := make([]string, 0)
	dscvr := metadbdiscovery.NewMetaDBBasedDiscovery(p.mDB)
	neighbourhood, err := dscvr.FindInShardOrSubcidByFQDN(ctx, fqdn)
	if err != nil {
		return result, err
	}
	result = append(result, strings.Split(fqdn, ".")[0])
	for _, h := range neighbourhood.Others {
		if neighbourhood.Self.Geo == h.Geo {
			result = append(result, strings.Split(h.FQDN, ".")[0])
		}
	}
	return result, nil
}
