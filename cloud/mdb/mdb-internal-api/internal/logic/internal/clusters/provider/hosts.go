package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Clusters) AddHosts(ctx context.Context, args []models.AddHostArgs) ([]hosts.Host, error) {
	// TODO: add validation
	res := make([]hosts.Host, 0, len(args))
	for _, arg := range args {
		host, err := c.metaDB.AddHost(ctx, arg)

		if err != nil {
			return nil, xerrors.Errorf("failed to add host to sub-cluster %s : %w", arg.SubClusterID, err)
		}

		res = append(res, host)
	}

	//TODO: Update used resources

	return res, nil
}

func (c *Clusters) ModifyHost(ctx context.Context, args models.ModifyHostArgs) error {
	_, err := c.metaDB.ModifyHost(ctx, args)
	return err
}

func (c *Clusters) ModifyHostPublicIP(ctx context.Context, clusterID string, fqdn string, revision int64, assignPublicIP bool) error {
	_, err := c.metaDB.ModifyHostPublicIP(ctx, clusterID, fqdn, revision, assignPublicIP)
	return err
}

func (c *Clusters) AnyHost(ctx context.Context, cid string) (hosts.HostExtended, error) {
	currentHosts, _, _, err := c.ListHosts(ctx, cid, 1, 0)
	if err != nil {
		return hosts.HostExtended{}, err
	}

	if len(currentHosts) == 0 {
		return hosts.HostExtended{}, xerrors.Errorf("cluster %q has no hosts", cid)
	}

	return currentHosts[0], nil
}

func (c *Clusters) ListHosts(ctx context.Context, cid string, pageSize int64, offset int64) ([]hosts.HostExtended, int64, bool, error) {
	pageSize = pagination.SanePageSize(pageSize)
	offset = pagination.SanePageOffset(offset)

	loaded, nextPageToken, more, err := c.metaDB.ListHosts(ctx, cid, offset, optional.NewInt64(pageSize))
	if err != nil {
		return nil, 0, false, err
	}

	var fqdns []string
	for _, host := range loaded {
		fqdns = append(fqdns, host.FQDN)
	}

	// Load health
	healths, err := c.health.Hosts(ctx, fqdns)
	if err != nil {
		c.l.Error("cannot retrieve hosts health", log.Error(err))
		sentry.GlobalClient().CaptureError(ctx, err, nil)
	}
	res := make([]hosts.HostExtended, 0, len(loaded))
	for _, host := range loaded {
		health, ok := healths[host.FQDN]
		if !ok {
			health = hosts.Health{
				Status:   hosts.StatusUnknown,
				Services: []services.Health{},
				System:   nil,
			}
		}

		res = append(res, hosts.HostExtended{Host: host, Health: health})
	}

	return res, nextPageToken, more, nil
}

func (c *Clusters) DeleteHosts(ctx context.Context, clusterID string, FQDNs []string, revision int64) ([]hosts.Host, error) {
	hsts, err := c.metaDB.DeleteHosts(ctx, clusterID, FQDNs, revision)
	if err != nil {
		return nil, xerrors.Errorf("failed to delete hosts from cluster %s and revision %d,  %w", clusterID, revision, err)
	}

	return hsts, nil
}
