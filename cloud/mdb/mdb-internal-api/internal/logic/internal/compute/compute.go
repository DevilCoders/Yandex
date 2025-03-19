package compute

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

//go:generate ../../../../../scripts/mockgen.sh Compute

type Compute interface {
	HostGroup(ctx context.Context, hostGroupID string) (compute.HostGroup, error)
	HostType(ctx context.Context, hostTypeID string) (compute.HostType, error)
	GetHostGroupHostType(ctx context.Context, hostTypeID []string) (map[string]compute.HostGroupHostType, error)
	Network(ctx context.Context, networkID string) (networkProvider.Network, error)
	Networks(ctx context.Context, projectID string, regionID string) ([]networkProvider.Network, error)
	CreateDefaultNetwork(ctx context.Context, projectID string, regionID string) (networkProvider.Network, error)
	Subnet(ctx context.Context, subnetID string) (networkProvider.Subnet, error)
	Subnets(ctx context.Context, network networkProvider.Network) ([]networkProvider.Subnet, error)
	NetworkAndSubnets(ctx context.Context, networkID string) (networkProvider.Network, []networkProvider.Subnet, error)
	PickSubnet(ctx context.Context, subnets []networkProvider.Subnet, vtype environment.VType, geo string, publicIP bool, requestedSubnetID optional.String, clusterFolderExtID string) (networkProvider.Subnet, error)
	ValidateHostGroups(ctx context.Context, hostGroupIDs []string, folderExtID string, cloudExtID string, zones []string) error
	ValidateSecurityGroups(ctx context.Context, securityGroupIDs []string, networkID string) error
}

func GetOrCreateNetwork(ctx context.Context, compute Compute, projectID string, regionID string) (networkProvider.Network, error) {
	nets, err := compute.Networks(ctx, projectID, regionID)
	if err != nil {
		return networkProvider.Network{}, err
	}

	if len(nets) > 1 {
		return networkProvider.Network{}, semerr.FailedPreconditionf("can't choose network, found %d networks", len(nets))
	}

	if len(nets) == 1 {
		return nets[0], nil
	}

	return compute.CreateDefaultNetwork(ctx, projectID, regionID)
}
