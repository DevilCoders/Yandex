package provider

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func (c *Compute) HostType(ctx context.Context, hostTypeID string) (compute.HostType, error) {
	if c.hostType == nil {
		return compute.HostType{}, semerr.InvalidInput("host type not fount")
	}
	hostType, err := c.hostType.Get(ctx, hostTypeID)
	if err != nil {
		return compute.HostType{}, translateComputeError(err, fmt.Sprintf("host group %q not found", hostTypeID))
	}
	return hostType, nil
}

func (c *Compute) GetHostGroupHostType(ctx context.Context, hostGroupIds []string) (map[string]compute.HostGroupHostType, error) {
	var err error
	hostGroupLen := len(hostGroupIds)
	hostGroupHostType := make(map[string]compute.HostGroupHostType, len(hostGroupIds))
	if hostGroupLen < 1 {
		return nil, err
	} else {
		for _, hostGroupID := range hostGroupIds {
			hostGroup, err := c.HostGroup(ctx, hostGroupID)
			if err != nil {
				return nil, err
			}
			hostType, err := c.HostType(ctx, hostGroup.TypeID)
			if err != nil {
				return nil, err
			}
			hostGroupType := compute.HostGroupHostType{
				HostGroup: hostGroup,
				HostType:  hostType,
			}
			hostGroupHostType[hostGroupID] = hostGroupType
		}
	}
	return hostGroupHostType, err
}
