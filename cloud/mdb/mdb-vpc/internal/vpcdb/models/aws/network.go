package aws

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

type NetworkState struct {
	ID          string `json:"id"`
	Name        string `json:"name"`
	Description string `json:"description"`
	IPv4        string `json:"ipv4"`
	IPv6        string `json:"ipv6"`
}

func NetworkToState(net models.Network) NetworkState {
	return NetworkState{
		ID:          net.ID,
		Name:        net.Name,
		Description: net.Description,
		IPv4:        net.IPv4,
		IPv6:        net.IPv6,
	}
}

type NetworkExternalResources struct {
	VpcID           string    `json:"vpc_id"`
	Subnets         []*Subnet `json:"subnets"`
	IgwID           string    `json:"igw_id"`
	SecurityGroupID string    `json:"security_group_id"`
	TgwAttachmentID string    `json:"tgw_attachment_id"`
	HostedZoneID    *string   `json:"hosted_zone_id"`

	AccountID  optional.String `json:"account_id"`
	IamRoleArn optional.String `json:"iam_role_arn"`
}

func DefaultNetworkExternalResources() models.ExternalResources {
	return &NetworkExternalResources{}
}

func (er *NetworkExternalResources) IsImported() bool {
	return er.IamRoleArn.Valid
}
