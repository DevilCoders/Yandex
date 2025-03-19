package hostpicker

import (
	"context"
	"fmt"

	healthclient "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type HostPicker interface {
	PickHost(ctx context.Context, fqdns []string) (string, error)
}

func AssertHostsHealth(ctx context.Context, health healthclient.MDBHealthClient, hosts []string) error {
	healths, err := health.GetHostsHealth(ctx, hosts)
	if err != nil {
		return err
	}
	unhealthy := make(map[string]types.HostStatus)
	for _, h := range healths {
		if h.Status() != types.HostStatusAlive {
			unhealthy[h.FQDN()] = h.Status()
		}
	}
	if len(unhealthy) > 0 {
		return fmt.Errorf("bad health status for hosts: %+v", unhealthy)
	}

	return nil
}
