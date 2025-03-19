package aws

import (
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

type NetworkConnectionType int

const (
	NetworkConnectionPeering NetworkConnectionType = iota
)

type NetworkConnectionParams struct {
	Type                NetworkConnectionType `json:"type"`
	AccountID           string                `json:"account_id"`
	VpcID               string                `json:"vpc_id"`
	IPv4                string                `json:"ipv4"`
	IPv6                string                `json:"ipv6"`
	Region              string                `json:"region"`
	PeeringConnectionID string                `json:"peering_connection_id"`
	ManagedIPv4         string                `json:"managed_ipv4,omitempty"`
	ManagedIPv6         string                `json:"managed_ipv6,omitempty"`
}

func NewNetworkConnectionPeeringParams(accountID, vpcID, ipv4, ipv6, region, peeringID string) models.NetworkConnectionParams {
	return &NetworkConnectionParams{
		Type:                NetworkConnectionPeering,
		AccountID:           accountID,
		VpcID:               vpcID,
		IPv4:                ipv4,
		IPv6:                ipv6,
		Region:              region,
		PeeringConnectionID: peeringID,
	}
}

func DefaultNetworkConnectionParams() models.NetworkConnectionParams {
	return &NetworkConnectionParams{}
}
