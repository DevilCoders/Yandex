package meta

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	"a.yandex-team.ru/cloud/mdb/internal/environment"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/network/doublecloud"
	"a.yandex-team.ru/cloud/mdb/internal/network/porto"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	vtype environment.VType

	l log.Logger

	vpc   vpc.Client
	porto *porto.Client
	aws   *doublecloud.Client
}

var _ networkProvider.Client = &Client{}

func NewClient(vtype string, vpc vpc.Client, porto *porto.Client, aws *doublecloud.Client, l log.Logger) (*Client, error) {
	parsedVType, err := environment.ParseVType(vtype)
	if err != nil {
		return &Client{}, xerrors.Errorf("can't parse vtype: %s", vtype)
	}
	return &Client{
		vtype: parsedVType,
		l:     l,
		vpc:   vpc,
		porto: porto,
		aws:   aws,
	}, nil
}

func (c *Client) GetNetwork(ctx context.Context, networkID string) (networkProvider.Network, error) {
	switch c.vtype {
	case environment.VTypePorto:
		return c.porto.GetNetwork(ctx, networkID)

	case environment.VTypeCompute:
		return c.vpc.GetNetwork(ctx, networkID)

	case environment.VTypeAWS:
		return c.aws.GetNetwork(ctx, networkID)

	default:
		return networkProvider.Network{}, xerrors.Errorf("unsupported vtype: %s", c.vtype)
	}
}

func (c *Client) GetNetworks(ctx context.Context, projectID, regionID string) ([]networkProvider.Network, error) {
	switch c.vtype {
	case environment.VTypeAWS:
		return c.aws.GetNetworks(ctx, projectID, regionID)
	default:
		return nil, xerrors.Errorf("unsupported vtype: %s", c.vtype)
	}
}

func (c *Client) CreateDefaultNetwork(ctx context.Context, projectID, regionID string) (networkProvider.Network, error) {
	switch c.vtype {
	case environment.VTypeAWS:
		return c.aws.CreateDefaultNetwork(ctx, projectID, regionID)
	default:
		return networkProvider.Network{}, xerrors.Errorf("unsupported vtype: %s", c.vtype)
	}
}

func (c *Client) GetSubnet(ctx context.Context, subnetID string) (networkProvider.Subnet, error) {
	switch c.vtype {
	case environment.VTypePorto:
		return c.porto.GetSubnet(ctx, subnetID)

	case environment.VTypeCompute:
		return c.vpc.GetSubnet(ctx, subnetID)

	case environment.VTypeAWS:
		return c.aws.GetSubnet(ctx, subnetID)

	default:
		return networkProvider.Subnet{}, xerrors.Errorf("unsupported vtype: %s", c.vtype)
	}
}

func (c *Client) GetSubnets(ctx context.Context, network networkProvider.Network) ([]networkProvider.Subnet, error) {
	switch c.vtype {
	case environment.VTypePorto:
		return c.porto.GetSubnets(ctx, network)

	case environment.VTypeCompute:
		return c.vpc.GetSubnets(ctx, network)

	case environment.VTypeAWS:
		return c.aws.GetSubnets(ctx, network)

	default:
		return nil, xerrors.Errorf("unsupported vtype: %s", c.vtype)
	}
}

func (c *Client) GetNetworksByCloudID(ctx context.Context, cloudID string) ([]string, error) {
	switch c.vtype {
	case environment.VTypePorto:
		return c.porto.GetNetworksByCloudID(ctx, cloudID)
	default:
		return nil, xerrors.Errorf("unsupported vtype: %s", c.vtype)
	}
}
