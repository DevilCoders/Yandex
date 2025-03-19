package metadbdiscovery

import (
	"context"
	"fmt"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (d *MetaDBBasedDiscovery) FindInShardOrSubcidByFQDN(ctx context.Context, fqdn string) (clusterdiscovery.Neighbourhood, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "FindInShardOrSubcidByFQDN", tags.InstanceFQDN.Tag(fqdn))
	defer span.Finish()
	result := clusterdiscovery.Neighbourhood{}
	metaDBctx, err := d.mDB.Begin(ctx, sqlutil.PreferStandby)
	if err != nil {
		return result, err
	}
	defer func() { _ = d.mDB.Rollback(metaDBctx) }()

	thisHost, err := d.mDB.GetHostByFQDN(metaDBctx, fqdn)
	if err != nil {
		if xerrors.Is(err, metadb.ErrDataNotFound) {
			return result, semerr.NotFoundf("host %s does not exist", fqdn)
		}
		return result, xerrors.Errorf("failed to get host by fqdn '%s': %w", fqdn, err)
	}

	var hosts []metadb.Host
	if thisHost.ShardID.Valid {
		result.ID = thisHost.ShardID.String
		hosts, err = d.mDB.GetHostsByShardID(metaDBctx, result.ID)
		if err != nil {
			return result, fmt.Errorf("could not get hosts in shard %s of '%s': %w",
				thisHost.ShardID.String, fqdn, err)
		}
	} else {
		result.ID = thisHost.SubClusterID
		hosts, err = d.mDB.GetHostsBySubcid(metaDBctx, result.ID)
		if err != nil {
			return result, fmt.Errorf("could not get hosts in subcluster %s of '%s': %w",
				thisHost.SubClusterID, fqdn, err)
		}
	}
	for _, host := range hosts {
		if host.FQDN == fqdn {
			result.Self = host
		} else {
			result.Others = append(result.Others, host)
		}
	}

	if err = d.mDB.Commit(metaDBctx); err != nil {
		return result, err
	}
	return result, nil
}
