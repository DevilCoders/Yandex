package grpc

import (
	"context"

	"google.golang.org/grpc/credentials"

	cloudVPC "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const pageSize = 500

// Client is gRPC client for VPC
type Client struct {
	l          log.Logger
	networkAPI cloudVPC.NetworkServiceClient
	subnetAPI  cloudVPC.SubnetServiceClient
	sgAPI      cloudVPC.SecurityGroupServiceClient
}

var _ vpc.Client = &Client{}

// NewClient constructs VPC client
func NewClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, l log.Logger) (*Client, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("connecting to VPC API at %q: %w", target, err)
	}

	return &Client{
		l:          l,
		networkAPI: cloudVPC.NewNetworkServiceClient(conn),
		subnetAPI:  cloudVPC.NewSubnetServiceClient(conn),
		sgAPI:      cloudVPC.NewSecurityGroupServiceClient(conn),
	}, nil
}

func networkFromGRPC(n *cloudVPC.Network) networkProvider.Network {
	r := networkProvider.Network{
		ID:          n.Id,
		FolderID:    n.FolderId,
		Name:        n.Name,
		Description: n.Description,
		Labels:      n.Labels,
	}
	if n.CreatedAt != nil {
		r.CreatedAt = n.CreatedAt.AsTime()
	}
	return r
}

func (c *Client) GetNetwork(ctx context.Context, networkID string) (networkProvider.Network, error) {
	if networkID == "" {
		return networkProvider.Network{}, semerr.InvalidInput("no network id provided")
	}
	resp, err := c.networkAPI.Get(ctx, &cloudVPC.GetNetworkRequest{NetworkId: networkID})
	if err != nil {
		return networkProvider.Network{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	return networkFromGRPC(resp), nil
}

func subnetFromGRPC(s *cloudVPC.Subnet) networkProvider.Subnet {
	r := networkProvider.Subnet{
		ID:           s.Id,
		FolderID:     s.FolderId,
		Name:         s.Name,
		Description:  s.Description,
		Labels:       s.Labels,
		NetworkID:    s.NetworkId,
		ZoneID:       s.ZoneId,
		V4CIDRBlocks: s.V4CidrBlocks,
		V6CIDRBlocks: s.V6CidrBlocks,
	}
	if s.CreatedAt != nil {
		r.CreatedAt = s.CreatedAt.AsTime()
	}
	return r
}

func (c *Client) GetSubnet(ctx context.Context, subnetID string) (networkProvider.Subnet, error) {
	if subnetID == "" {
		return networkProvider.Subnet{}, semerr.InvalidInput("no subnet id provided")
	}
	resp, err := c.subnetAPI.Get(ctx, &cloudVPC.GetSubnetRequest{SubnetId: subnetID})
	if err != nil {
		return networkProvider.Subnet{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	return subnetFromGRPC(resp), nil
}

func (c *Client) GetSubnets(ctx context.Context, network networkProvider.Network) ([]networkProvider.Subnet, error) {
	var subnets []networkProvider.Subnet
	var pageToken string
	for {
		resp, err := c.networkAPI.ListSubnets(ctx, &cloudVPC.ListNetworkSubnetsRequest{
			NetworkId: network.ID,
			PageSize:  pageSize,
			PageToken: pageToken,
		})
		if err != nil {
			return nil, grpcerr.SemanticErrorFromGRPC(err)
		}
		for _, s := range resp.Subnets {
			subnets = append(subnets, subnetFromGRPC(s))
		}

		if len(resp.NextPageToken) == 0 {
			break
		}
		pageToken = resp.NextPageToken
	}
	return subnets, nil
}

func sgFromGRPC(sg *cloudVPC.SecurityGroup) (vpc.SecurityGroup, error) {
	if sg == nil {
		return vpc.SecurityGroup{}, xerrors.New("fail to convert security group from gRPC, cause SecurityGroup is nil")
	}
	rules := make([]vpc.SecurityGroupRule, len(sg.Rules))
	for i, r := range sg.Rules {
		rules[i] = vpc.SecurityGroupRule{
			ID:             r.Id,
			Description:    r.Description,
			ProtocolName:   r.ProtocolName,
			ProtocolNumber: r.ProtocolNumber,
		}
		switch r.Direction {
		case cloudVPC.SecurityGroupRule_EGRESS:
			rules[i].Direction = vpc.SecurityGroupEgressDirection
		case cloudVPC.SecurityGroupRule_INGRESS:
			rules[i].Direction = vpc.SecurityGroupIngressDirection
		}
		if r.Ports != nil {
			rules[i].PortsFrom = r.Ports.FromPort
			rules[i].PortsTo = r.Ports.ToPort
		}
		if cidrBlocks := r.GetCidrBlocks(); cidrBlocks != nil {
			rules[i].V4CidrBlocks = cidrBlocks.V4CidrBlocks
			rules[i].V6CidrBlocks = cidrBlocks.V6CidrBlocks
		}
	}
	return vpc.SecurityGroup{
		ID:                sg.Id,
		Name:              sg.Name,
		FolderID:          sg.FolderId,
		NetworkID:         sg.NetworkId,
		DefaultForNetwork: sg.DefaultForNetwork,
		Rules:             rules,
	}, nil
}

func (c *Client) GetSecurityGroup(ctx context.Context, securityGroupID string) (vpc.SecurityGroup, error) {
	if securityGroupID == "" {
		return vpc.SecurityGroup{}, semerr.InvalidInput("no security group id provided")
	}
	resp, err := c.sgAPI.Get(ctx, &cloudVPC.GetSecurityGroupRequest{SecurityGroupId: securityGroupID})
	if err != nil {
		return vpc.SecurityGroup{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	return sgFromGRPC(resp)
}
