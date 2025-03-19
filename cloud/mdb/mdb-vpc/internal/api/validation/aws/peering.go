package aws

import (
	"context"
	"net"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (v *Validator) ValidateCreateNetworkConnectionData(ctx context.Context, netObj models.Network, params interface{}) (validation.ValidateNetworkConnectionCreationResult, error) {
	res := validation.ValidateNetworkConnectionCreationResult{}
	awsParams := params.(*network.CreateNetworkConnectionRequest_Aws).Aws
	peeringParams := awsParams.Type.(*network.CreateAWSNetworkConnectionRequest_Peering).Peering

	ip, _, err := net.ParseCIDR(peeringParams.Ipv4CidrBlock)
	if err != nil {
		return res, xerrors.Errorf("invalid IvP4 CIDR %q: %w", peeringParams.Ipv4CidrBlock, err)
	}
	if ip.To4() == nil {
		return res, xerrors.Errorf("%q is not a valid IPv4 CIDR", peeringParams.Ipv4CidrBlock)
	}

	if v6CIDR := peeringParams.Ipv6CidrBlock; v6CIDR != "" {
		ip, _, err := net.ParseCIDR(v6CIDR)
		if err != nil {
			return res, xerrors.Errorf("invalid IvP6 CIDR %q: %w", v6CIDR, err)
		}
		if ip.To4() != nil {
			return res, xerrors.Errorf("%q is not a valid IPv6 CIDR", v6CIDR)
		}
	}

	ec2Cli := v.ec2CliFactory(v.defaultDataplaneIAMRole, netObj.Region, v.client)
	input := &ec2.CreateVpcPeeringConnectionInput{
		PeerRegion:  aws.String(peeringParams.RegionId),
		PeerVpcId:   aws.String(peeringParams.VpcId),
		PeerOwnerId: aws.String(peeringParams.AccountId),
		VpcId:       aws.String(netObj.ExternalResources.(*awsmodels.NetworkExternalResources).VpcID),
	}
	data, err := ec2Cli.CreateVpcPeeringConnectionWithContext(ctx, input)
	if err != nil {
		return res, xerrors.Errorf("create peering connection: %w", err)
	}
	res.PeeringID = aws.StringValue(data.VpcPeeringConnection.VpcPeeringConnectionId)

	return res, nil
}
