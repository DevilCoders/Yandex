package aws

import (
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	IsNotAvailable = xerrors.New("is not available")
)

func VpcToDcNetwork(v *ec2.Vpc) (aws2.Vpc, error) {
	if aws.StringValue(v.State) != "available" {
		return aws2.Vpc{}, IsNotAvailable
	}
	if len(v.Ipv6CidrBlockAssociationSet) == 0 {
		return aws2.Vpc{}, xerrors.New("there are no ipv6 associations")
	}
	return aws2.Vpc{
		ID:       aws.StringValue(v.VpcId),
		IPv4CIDR: aws.StringValue(v.CidrBlock),
		IPv6CIDR: aws.StringValue(v.Ipv6CidrBlockAssociationSet[0].Ipv6CidrBlock),
	}, nil
}

func SubnetToDcSubnet(s *ec2.Subnet) (aws2.Subnet, error) {
	if aws.StringValue(s.State) != "available" {
		return aws2.Subnet{}, IsNotAvailable
	}
	if len(s.Ipv6CidrBlockAssociationSet) == 0 {
		return aws2.Subnet{}, xerrors.New("there are no ipv6 associations")
	}

	return aws2.Subnet{
		ID:       aws.StringValue(s.SubnetId),
		IPv4CIDR: aws.StringValue(s.CidrBlock),
		IPv6CIDR: aws.StringValue(s.Ipv6CidrBlockAssociationSet[0].Ipv6CidrBlock),
		AZ:       aws.StringValue(s.AvailabilityZoneId),
	}, nil
}

func ipv4RangesToAws(ranges []string, description string) []*ec2.IpRange {
	res := make([]*ec2.IpRange, len(ranges))
	desc := aws.String(description)
	for i, rang := range ranges {
		res[i] = &ec2.IpRange{
			CidrIp:      aws.String(rang),
			Description: desc,
		}
	}
	return res
}

func ipv6RangesToAws(ranges []string, description string) []*ec2.Ipv6Range {
	res := make([]*ec2.Ipv6Range, len(ranges))
	desc := aws.String(description)
	for i, rang := range ranges {
		res[i] = &ec2.Ipv6Range{
			CidrIpv6:    aws.String(rang),
			Description: desc,
		}
	}
	return res
}
