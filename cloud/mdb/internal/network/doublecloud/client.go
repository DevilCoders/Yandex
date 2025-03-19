package doublecloud

import (
	"context"
	"fmt"

	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth/iamauth"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Client is interface to network.api
type Client struct {
	l                    log.Logger
	apiClient            apiv1.NetworkServiceClient
	defaultIPv4CidrBlock string
}

var _ network.Client = &Client{}

// NewClient constructs network.client
func NewClient(ctx context.Context, addr, userAgent, defaultIPv4CidrBlock string, config grpcutil.ClientConfig, l log.Logger) (*Client, error) {
	conn, err := grpcutil.NewConn(ctx, addr, userAgent, config, l, grpcutil.WithClientCredentials(grpcauth.NewContextCredentials(iamauth.NewIAMAuthTokenModel())))
	if err != nil {
		return nil, xerrors.Errorf("connecting to network.API at %q: %w", addr, err)
	}

	return &Client{
		l:                    l,
		apiClient:            apiv1.NewNetworkServiceClient(conn),
		defaultIPv4CidrBlock: defaultIPv4CidrBlock,
	}, nil
}

func networkFromGRPC(n *apiv1.Network) network.Network {
	return network.Network{
		ID:           n.Id,
		FolderID:     n.ProjectId,
		Name:         n.Name,
		Description:  n.Description,
		V4CIDRBlocks: []string{n.Ipv4CidrBlock},
		V6CIDRBlocks: []string{n.Ipv6CidrBlock},
	}
}

func (c *Client) GetNetwork(ctx context.Context, networkID string) (network.Network, error) {
	resp, err := c.apiClient.Get(ctx, &apiv1.GetNetworkRequest{
		NetworkId: networkID,
	})

	if err != nil {
		return network.Network{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	return networkFromGRPC(resp), nil
}

func (c *Client) GetNetworks(ctx context.Context, projectID string, region string) ([]network.Network, error) {
	resp, err := c.apiClient.List(ctx, &apiv1.ListNetworksRequest{
		ProjectId: projectID,
	})

	if err != nil {
		return nil, grpcerr.SemanticErrorFromGRPC(err)
	}

	nets := make([]network.Network, 0, len(resp.GetNetworks()))
	for _, net := range resp.GetNetworks() {
		if net.GetRegionId() == region {
			nets = append(nets, networkFromGRPC(net))
		}
	}

	return nets, nil
}

func (c *Client) CreateDefaultNetwork(ctx context.Context, projectID string, region string) (network.Network, error) {
	resp, err := c.apiClient.Create(ctx, &apiv1.CreateNetworkRequest{
		ProjectId:     projectID,
		CloudType:     "aws",
		RegionId:      region,
		Name:          fmt.Sprintf("default in %s", region),
		Description:   fmt.Sprintf("auto created network for %s in %s", projectID, region),
		Ipv4CidrBlock: c.defaultIPv4CidrBlock,
	})
	if err != nil {
		return network.Network{}, err
	}

	return c.GetNetwork(ctx, resp.NetworkId)
}

func (c *Client) GetSubnet(ctx context.Context, subnetID string) (network.Subnet, error) {
	return network.Subnet{}, xerrors.Errorf("unavailable GetSubnet")
}

func (c *Client) GetSubnets(ctx context.Context, net network.Network) ([]network.Subnet, error) {
	resp, err := c.apiClient.Get(ctx, &apiv1.GetNetworkRequest{
		NetworkId: net.ID,
	})

	if err != nil {
		return nil, grpcerr.SemanticErrorFromGRPC(err)
	}

	switch res := resp.ExternalResources.(type) {
	case *apiv1.Network_Aws:
		subnets := make([]network.Subnet, 0, len(res.Aws.Subnets))
		for _, s := range res.Aws.Subnets {
			subnets = append(subnets, network.Subnet{
				ID:       s.Id,
				ZoneID:   s.ZoneId,
				FolderID: net.FolderID,
			})
		}

		return subnets, nil
	default:
		return nil, xerrors.Errorf("unavailable external resource type %v", res)
	}
}

func (c *Client) GetNetworksByCloudID(ctx context.Context, cloudID string) ([]string, error) {
	return nil, xerrors.Errorf("unavailable GetNetworksByCloudID")
}
