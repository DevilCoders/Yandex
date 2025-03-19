package nop

import (
	"context"

	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
)

type Client struct{}

var _ networkProvider.Client = &Client{}

func (c *Client) GetSubnets(ctx context.Context, network networkProvider.Network) ([]networkProvider.Subnet, error) {
	return nil, nil
}

func (c *Client) GetNetwork(ctx context.Context, networkID string) (networkProvider.Network, error) {
	return networkProvider.Network{}, nil
}

func (c *Client) GetNetworks(ctx context.Context, projectID, regionID string) ([]networkProvider.Network, error) {
	return nil, nil
}

func (c *Client) CreateDefaultNetwork(ctx context.Context, projectID, regionID string) (networkProvider.Network, error) {
	return networkProvider.Network{}, nil
}

func (c *Client) GetSubnet(ctx context.Context, subnetID string) (networkProvider.Subnet, error) {
	return networkProvider.Subnet{}, nil
}

func (c *Client) GetNetworksByCloudID(ctx context.Context, cloudID string) ([]string, error) {
	return nil, nil
}
