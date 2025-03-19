package grpcdiscovery

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
)

type Discovery struct {
	client instanceclient.InstanceClient
}

func (d *Discovery) Dom0Instances(ctx context.Context, dom0 string) (dscv dom0discovery.DiscoveryResult, err error) {
	data, err := d.client.Dom0Instances(ctx, dom0)
	if err != nil {
		return dom0discovery.DiscoveryResult{}, err
	}

	res := dom0discovery.DiscoveryResult{
		WellKnown: make([]models.Instance, len(data.WellKnown)),
		Unknown:   data.Unknown,
	}
	for i, instance := range data.WellKnown {
		volumes := make([]dbm.Volume, 0, len(instance.Volumes))
		for _, v := range instance.Volumes {
			volumes = append(volumes, dbm.Volume{SpaceLimit: int(v)})
		}

		res.WellKnown[i] = models.Instance{
			ConductorGroup: instance.ConductorGroup,
			DBMClusterName: instance.DbmClusterName,
			FQDN:           instance.Fqdn,
			Volumes:        volumes,
		}
	}

	return res, nil
}

func NewDiscovery(client instanceclient.InstanceClient) dom0discovery.Dom0Discovery {
	return &Discovery{client: client}
}
