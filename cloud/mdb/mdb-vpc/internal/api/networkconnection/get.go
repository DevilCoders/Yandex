package networkconnection

import (
	"context"

	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
)

func (s Service) Get(ctx context.Context, request *network.GetNetworkConnectionRequest) (*network.NetworkConnection, error) {
	nc, err := s.db.NetworkConnectionByID(ctx, request.NetworkConnectionId)
	if err != nil {
		return nil, err
	}

	if _, err := s.auth.Authorize(ctx, auth.NetworkConnectionGetPermission, nc.ProjectID); err != nil {
		return nil, err
	}
	return networkConnectionFromDB(nc), nil
}

func networkConnectionFromDB(nc models.NetworkConnection) *network.NetworkConnection {
	var status network.NetworkConnection_NetworkConnectionStatus
	switch nc.Status {
	case models.NetworkConnectionStatusCreating:
		status = network.NetworkConnection_NETWORK_CONNECTION_STATUS_CREATING
	case models.NetworkConnectionStatusActive:
		status = network.NetworkConnection_NETWORK_CONNECTION_STATUS_ACTIVE
	case models.NetworkConnectionStatusDeleting:
		status = network.NetworkConnection_NETWORK_CONNECTION_STATUS_DELETING
	case models.NetworkConnectionStatusPending:
		status = network.NetworkConnection_NETWORK_CONNECTION_STATUS_PENDING
	case models.NetworkConnectionStatusError:
		status = network.NetworkConnection_NETWORK_CONNECTION_STATUS_ERROR
	}

	res := &network.NetworkConnection{
		Id:             nc.ID,
		NetworkId:      nc.NetworkID,
		ConnectionInfo: nil,
		CreateTime:     timestamppb.New(nc.CreateTime),
		Description:    nc.Description,
		Status:         status,
		StatusReason:   nc.StatusReason,
	}

	switch nc.Provider {
	case models.ProviderAWS:
		connInfo := &network.NetworkConnection_Aws{Aws: &network.AWSNetworkConnectionInfo{}}
		params := nc.Params.(*aws.NetworkConnectionParams)
		switch params.Type {
		case aws.NetworkConnectionPeering:
			connInfo.Aws.Type = &network.AWSNetworkConnectionInfo_Peering{
				Peering: &network.AWSNetworkConnectionPeeringInfo{
					VpcId:                params.VpcID,
					AccountId:            params.AccountID,
					RegionId:             params.Region,
					Ipv4CidrBlock:        params.IPv4,
					Ipv6CidrBlock:        params.IPv6,
					PeeringConnectionId:  params.PeeringConnectionID,
					ManagedIpv4CidrBlock: params.ManagedIPv4,
					ManagedIpv6CidrBlock: params.ManagedIPv6,
				},
			}
		}
		res.ConnectionInfo = connInfo
	}

	return res
}
