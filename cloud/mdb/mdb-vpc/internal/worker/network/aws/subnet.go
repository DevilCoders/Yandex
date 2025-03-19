package aws

import (
	"context"
	"math"
	"net"

	"github.com/apparentlymart/go-cidr/cidr"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws/ready"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	defaultNewBits float64 = 2
)

func (s *Service) GetSubnet(ctx context.Context, id string) (aws2.Subnet, error) {
	res, err := s.providers.Dataplane.Ec2.DescribeSubnetsWithContext(ctx, &ec2.DescribeSubnetsInput{SubnetIds: aws.StringSlice([]string{id})})
	if err != nil {
		return aws2.Subnet{}, err
	}
	if len(res.Subnets) == 0 {
		return aws2.Subnet{}, semerr.NotFound("vpc doesn't exist")
	}
	return SubnetToDcSubnet(res.Subnets[0])
}

func (s *Service) CreateSubnets(
	ctx context.Context,
	op models.Operation,
	state *aws2.CreateNetworkOperationState,
) error {
	if state.Subnets == nil {
		state.Subnets = make([]*aws2.Subnet, len(state.Zones))
	}

	/*
		/18 networks for 4 and less AZs
		/19 networks up to 8 AZs
		/20 networks for 9 and more AZs
	*/
	newBits := int(math.Max(defaultNewBits, math.Ceil(math.Log2(float64(len(state.Zones))))))
	for i, zoneID := range state.Zones {
		if state.Subnets[i] != nil {
			continue
		}

		ipv4Subnet, err := CreateSubnetCIDR(state.Vpc.IPv4CIDR, newBits, i)
		if err != nil {
			return xerrors.Errorf("create ipv4 subnet cidr: %w", err)
		}
		ipv6Subnet, err := CreateSubnetCIDR(state.Vpc.IPv6CIDR, 8, i)
		if err != nil {
			return xerrors.Errorf("create ipv6 subnet cidr: %w", err)
		}
		input := &ec2.CreateSubnetInput{
			AvailabilityZoneId: aws.String(zoneID),
			VpcId:              aws.String(state.Vpc.ID),
			CidrBlock:          aws.String(ipv4Subnet),
			Ipv6CidrBlock:      aws.String(ipv6Subnet),
			TagSpecifications:  s.NetworkTags("subnet", op.ProjectID, state.Network.Name, state.Network.ID),
		}
		var subnetObj *ec2.Subnet
		sn, err := s.providers.Dataplane.Ec2.CreateSubnetWithContext(ctx, input)
		if err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != SubnetAlreadyExists {
				return xerrors.Errorf("create subnet in %q: %w", zoneID, err)
			}
			snts, err := s.providers.Dataplane.Ec2.DescribeSubnetsWithContext(ctx, &ec2.DescribeSubnetsInput{
				Filters: []*ec2.Filter{
					{
						Name:   aws.String("availability-zone-id"),
						Values: aws.StringSlice([]string{zoneID}),
					},
					{
						Name:   aws.String("vpc-id"),
						Values: aws.StringSlice([]string{state.Vpc.ID}),
					},
				},
			})
			if err != nil {
				return xerrors.Errorf("describe subnets: %w", err)
			}
			if len(snts.Subnets) != 1 {
				return xerrors.Errorf("there are %d subnets, expect 1", len(snts.Subnets))
			}
			subnetObj = snts.Subnets[0]
		} else {
			subnetObj = sn.Subnet
		}

		subnet, err := SubnetToDcSubnet(subnetObj)
		if err != nil {
			if xerrors.Is(err, IsNotAvailable) {
				subnetID := aws.StringValue(sn.Subnet.SubnetId)
				subnet, err = ready.WaitForSubnetReady(ctx, s.GetSubnet, IsNotAvailable, subnetID, s.logger)
			}
		}

		if err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != SubnetAlreadyExists {
				return xerrors.Errorf("await subnet %q: %w", subnet.ID, err)
			}
		}

		if _, err = s.providers.Dataplane.Ec2.ModifySubnetAttributeWithContext(ctx, &ec2.ModifySubnetAttributeInput{
			AssignIpv6AddressOnCreation: &ec2.AttributeBooleanValue{Value: aws.Bool(true)},
			SubnetId:                    aws.String(subnet.ID),
		}); err != nil {
			return xerrors.Errorf("update AssignIpv6AddressOnCreation %q: %w", subnet.ID, err)
		}

		if _, err = s.providers.Dataplane.Ec2.ModifySubnetAttributeWithContext(ctx, &ec2.ModifySubnetAttributeInput{
			MapPublicIpOnLaunch: &ec2.AttributeBooleanValue{Value: aws.Bool(true)},
			SubnetId:            aws.String(subnet.ID),
		}); err != nil {
			return xerrors.Errorf("update MapPublicIpOnLaunch %q: %w", subnet.ID, err)
		}

		state.Subnets[i] = &subnet
	}

	return nil
}

func CreateSubnetCIDR(baseCidr string, newBits int, num int) (string, error) {
	_, base, err := net.ParseCIDR(baseCidr)
	if err != nil {
		return "", xerrors.Errorf("parse cidr: %w", err)
	}
	subnet, err := cidr.Subnet(base, newBits, num)
	if err != nil {
		return "", xerrors.Errorf("make subnet cidr: %w", err)
	}
	return subnet.String(), nil
}

func (s *Service) DeleteSubnets(
	ctx context.Context,
	op models.Operation,
	state *aws2.DeleteNetworkOperationState,
) error {
	for i, subnet := range state.Subnets {
		if subnet == nil {
			continue
		}
		input := &ec2.DeleteSubnetInput{SubnetId: aws.String(subnet.ID)}
		_, err := s.providers.Dataplane.Ec2.DeleteSubnetWithContext(ctx, input)
		if err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != SubnetNotFound {
				return xerrors.Errorf("delete subnet %q: %w", subnet.ID, err)
			}
		}
		ctxlog.Debugf(ctx, s.logger, "Deleted subnet %q in AZ %q", subnet.ID, subnet.AZ)
		state.Subnets[i] = nil
	}
	state.Subnets = nil
	return nil
}
