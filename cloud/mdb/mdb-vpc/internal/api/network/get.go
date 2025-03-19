package network

import (
	"context"

	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
)

func (s Service) Get(ctx context.Context, request *network.GetNetworkRequest) (*network.Network, error) {
	if request.GetNetworkId() == "" {
		return nil, semerr.InvalidInput("empty network id")
	}

	net, err := s.db.NetworkByID(ctx, request.NetworkId)
	if err != nil {
		return nil, err
	}

	if _, err = s.auth.Authorize(ctx, auth.NetworkGetPermission, net.ProjectID); err != nil {
		return nil, err
	}

	return networkFromDB(net), nil
}

func networkFromDB(net models.Network) *network.Network {
	var status network.Network_NetworkStatus
	switch net.Status {
	case models.NetworkStatusCreating:
		status = network.Network_NETWORK_STATUS_CREATING
	case models.NetworkStatusActive:
		status = network.Network_NETWORK_STATUS_ACTIVE
	case models.NetworkStatusDeleting:
		status = network.Network_NETWORK_STATUS_DELETING
	}

	res := &network.Network{
		Id:            net.ID,
		ProjectId:     net.ProjectID,
		CloudType:     string(net.Provider),
		RegionId:      net.Region,
		CreateTime:    timestamppb.New(net.CreateTime),
		Name:          net.Name,
		Description:   net.Description,
		Ipv4CidrBlock: net.IPv4,
		Ipv6CidrBlock: net.IPv6,
		Status:        status,
		StatusReason:  net.StatusReason,
	}

	switch net.Provider {
	case models.ProviderAWS:
		resources := net.ExternalResources.(*aws.NetworkExternalResources)
		subnets := make([]*network.AwsExternalResources_Subnet, len(resources.Subnets))
		for i, subnet := range resources.Subnets {
			subnets[i] = &network.AwsExternalResources_Subnet{
				ZoneId: subnet.AZ,
				Id:     subnet.ID,
			}
		}
		res.ExternalResources = &network.Network_Aws{
			Aws: &network.AwsExternalResources{
				VpcId:           resources.VpcID,
				Subnets:         subnets,
				SecurityGroupId: resources.SecurityGroupID,
				AccountId:       optional.StringToGRPC(resources.AccountID),
				IamRoleArn:      optional.StringToGRPC(resources.IamRoleArn),
			}}
	}

	return res
}
