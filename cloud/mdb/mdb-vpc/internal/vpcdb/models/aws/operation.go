package aws

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

type CreateNetworkOperationState struct {
	IsInited            bool      `json:"is_inited"`
	Vpc                 *Vpc      `json:"vpc"`
	Subnets             []*Subnet `json:"subnets"`
	Zones               []string  `json:"zones"`
	SubnetIDs           []string  `json:"subnet_ids"`
	PeeringID           string    `json:"peering_id"`
	IgwID               string    `json:"igw_id"`
	SecurityGroupID     string    `json:"security_group_id"`
	EgressRoutesCreated bool      `json:"egress_routes_created"`
	NlbRoutesCreated    bool      `json:"nlb_routes_created"`
	VpcRouteTableID     string    `json:"vpc_route_table_id"`
	TgwAttachmentID     string    `json:"tgw_attachment_id" yaml:"tgw_attachment_id"`
	TgwAssociated       bool      `json:"tgw_associated" yaml:"tgw_associated"`
	TgwPropEnabled      bool      `json:"tgw_prop_enabled" yaml:"tgw_prop_enabled"`
	HostedZoneID        string    `json:"hosted_zone_id" yaml:"hosted_zone_id"`

	Network NetworkState `json:"network" yaml:"network"`
}

type DeleteNetworkOperationState struct {
	IsInited        bool      `json:"is_inited"`
	VpcID           string    `json:"vpc_id"`
	Subnets         []*Subnet `json:"subnets"`
	IgwID           string    `json:"igw_id"`
	SecurityGroupID string    `json:"security_group_id"`
	TgwAttachmentID string    `json:"tgw_attachment_id" yaml:"tgw_attachment_id"`
	HostedZoneID    *string   `json:"hosted_zone_id" yaml:"hosted_zone_id"`

	AreRoutesDeleted bool `json:"are_routes_deleted"`

	VpcRouteTableID string `json:"vpc_route_table_id"`

	AccountID   optional.String `json:"account_id"`
	IamRoleArn  optional.String `json:"iam_role_arn"`
	TgwUnshared bool            `json:"tgw_unshared"`
}

type CreateNetworkConnectionOperationState struct {
	IsInited             bool   `json:"is_inited"`
	VpcID                string `json:"vpc_id"`
	VpcRouteTableID      string `json:"vpc_route_table_id"`
	PeeringRoutesCreated bool   `json:"peering_routes_created"`
	IsMarkedPending      bool   `json:"is_marked_pending"`

	NetworkConnectionParams *NetworkConnectionParams `json:"network_connection_params"`

	Network           NetworkState             `json:"network" yaml:"network"`
	NetworkConnection models.NetworkConnection `json:"network_connection"`
}

type DeleteNetworkConnectionOperationState struct {
	IsInited             bool   `json:"is_inited"`
	PeeringID            string `json:"peering_id"`
	VpcID                string `json:"vpc_id"`
	VpcRouteTableID      string `json:"vpc_route_table_id"`
	PeeringRoutesDeleted bool   `json:"peering_routes_deleted"`

	NetworkConnectionParams *NetworkConnectionParams `json:"network_connection_params"`

	Network           NetworkState             `json:"network" yaml:"network"`
	NetworkConnection models.NetworkConnection `json:"network_connection"`
}

type ImportVPCOperationParams struct {
	NetworkID  string `json:"network_id"`
	VpcID      string `json:"vpc_id"`
	IamRoleArn string `json:"iam_role_arn"`
	AccountID  string `json:"account_id"`
}

type ImportVPCOperationState struct {
	CreateNetworkOperationState
	AccountID  string `json:"account_id"`
	TgwShared  bool   `json:"tgw_shared"`
	IamRoleArn string `json:"iam_role_arn"`
}

func DefaultCreateNetworkOperationState() models.OperationState {
	return &CreateNetworkOperationState{}
}

func DefaultDeleteNetworkOperationState() models.OperationState {
	return &DeleteNetworkOperationState{}
}

func DefaultCreateNetworkConnectionOperationState() models.OperationState {
	return &CreateNetworkConnectionOperationState{}
}

func DefaultDeleteNetworkConnectionOperationState() models.OperationState {
	return &DeleteNetworkConnectionOperationState{}
}

func DefaultImportVPCOperationParams() models.OperationParams {
	return &ImportVPCOperationParams{}
}

func DefaultImportVPCOperationState() models.OperationState {
	return &ImportVPCOperationState{}
}

type Vpc struct {
	ID       string `json:"id"`
	IPv4CIDR string `json:"ipv4_cidr"`
	IPv6CIDR string `json:"ipv6_cidr"`
}

type Subnet struct {
	ID       string `json:"id"`
	IPv4CIDR string `json:"ipv4_cidr"`
	IPv6CIDR string `json:"ipv6_cidr"`
	AZ       string `json:"az"`
}
