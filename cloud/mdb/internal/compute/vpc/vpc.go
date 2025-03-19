package vpc

import (
	"context"

	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
)

//go:generate ../../../scripts/mockgen.sh Client

type SecurityGroupRuleDirection int

const (
	SecurityGroupUnspecifiedDirection SecurityGroupRuleDirection = iota
	SecurityGroupIngressDirection
	SecurityGroupEgressDirection
)

// SecurityGroupRule ...
type SecurityGroupRule struct {
	ID          string
	Description string
	Direction   SecurityGroupRuleDirection
	PortsFrom   int64
	PortsTo     int64
	// values from https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
	ProtocolName string
	// values from https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
	ProtocolNumber int64
	V4CidrBlocks   []string
	V6CidrBlocks   []string
}

// SecurityGroup ...
type SecurityGroup struct {
	ID                string
	Name              string
	FolderID          string
	NetworkID         string
	DefaultForNetwork bool
	Rules             []SecurityGroupRule
}

// Client for YC Virtual Private Cloud
type Client interface {
	GetSubnets(ctx context.Context, network networkProvider.Network) ([]networkProvider.Subnet, error)
	GetNetwork(ctx context.Context, networkID string) (networkProvider.Network, error)
	GetSubnet(ctx context.Context, subnetID string) (networkProvider.Subnet, error)
	GetSecurityGroup(ctx context.Context, securityGroupID string) (SecurityGroup, error)
}
