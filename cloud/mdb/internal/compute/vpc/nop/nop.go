package nop

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
)

type Client struct{}

var _ vpc.Client = &Client{}

func (c *Client) GetSubnets(ctx context.Context, network networkProvider.Network) ([]networkProvider.Subnet, error) {
	return nil, nil
}

func (c *Client) GetNetwork(ctx context.Context, networkID string) (networkProvider.Network, error) {
	return networkProvider.Network{}, nil
}

func (c *Client) GetSubnet(ctx context.Context, subnetID string) (networkProvider.Subnet, error) {
	return networkProvider.Subnet{}, nil
}

func (c *Client) GetSecurityGroup(ctx context.Context, securityGroupID string) (vpc.SecurityGroup, error) {
	return vpc.SecurityGroup{}, nil
}
