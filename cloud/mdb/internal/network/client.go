package network

import (
	"context"
)

//go:generate ../../scripts/mockgen.sh Client

type Client interface {
	GetSubnet(ctx context.Context, subnetID string) (Subnet, error)
	GetSubnets(ctx context.Context, network Network) ([]Subnet, error)
	GetNetwork(ctx context.Context, networkID string) (Network, error)
	GetNetworks(ctx context.Context, projectID, regionID string) ([]Network, error)
	CreateDefaultNetwork(ctx context.Context, projectID, regionID string) (Network, error)
	GetNetworksByCloudID(ctx context.Context, cloudID string) ([]string, error)
}
