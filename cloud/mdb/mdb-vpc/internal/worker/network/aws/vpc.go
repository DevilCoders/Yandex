package aws

import (
	"context"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws/ready"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) GetVpc(ctx context.Context, id string) (aws2.Vpc, error) {
	res, err := s.providers.Dataplane.Ec2.DescribeVpcsWithContext(ctx, &ec2.DescribeVpcsInput{VpcIds: aws.StringSlice([]string{id})})
	if err != nil {
		return aws2.Vpc{}, err
	}
	if len(res.Vpcs) == 0 {
		return aws2.Vpc{}, semerr.NotFound("vpc doesn't exist")
	}
	return VpcToDcNetwork(res.Vpcs[0])
}

func (s *Service) CreateVpc(
	ctx context.Context,
	op models.Operation,
	state *aws2.CreateNetworkOperationState,
) error {
	if state.Vpc != nil {
		return nil
	}

	input := &ec2.CreateVpcInput{
		AmazonProvidedIpv6CidrBlock: aws.Bool(true),
		CidrBlock:                   aws.String(state.Network.IPv4),
		TagSpecifications:           s.NetworkTags(ec2.ResourceTypeVpc, op.ProjectID, state.Network.Name, state.Network.ID),
	}
	res, err := s.providers.Dataplane.Ec2.CreateVpcWithContext(ctx, input)
	if err != nil {
		return xerrors.Errorf("create vpc: %w", err)
	}

	net, err := VpcToDcNetwork(res.Vpc)
	if err != nil && xerrors.Is(err, IsNotAvailable) {
		vpcID := aws.StringValue(res.Vpc.VpcId)
		net, err = ready.WaitForVPCReady(ctx, s.GetVpc, IsNotAvailable, vpcID, s.logger)
	}
	if err != nil {
		return xerrors.Errorf("await vpc: %w", err)
	}

	state.Vpc = &net

	return nil
}

func (s *Service) TuneVpc(
	ctx context.Context,
	op models.Operation,
	state *aws2.CreateNetworkOperationState,
) error {
	if _, err := s.providers.Dataplane.Ec2.ModifyVpcAttributeWithContext(ctx, &ec2.ModifyVpcAttributeInput{
		EnableDnsHostnames: &ec2.AttributeBooleanValue{Value: aws.Bool(true)},
		VpcId:              aws.String(state.Vpc.ID),
	}); err != nil {
		return xerrors.Errorf("modify EnableDnsHostnames on %q: %w", state.Vpc.ID, err)
	}
	if _, err := s.providers.Dataplane.Ec2.ModifyVpcAttributeWithContext(ctx, &ec2.ModifyVpcAttributeInput{
		EnableDnsSupport: &ec2.AttributeBooleanValue{Value: aws.Bool(true)},
		VpcId:            aws.String(state.Vpc.ID),
	}); err != nil {
		return xerrors.Errorf("modify EnableDnsSupport on %q: %w", state.Vpc.ID, err)
	}
	return nil
}

func (s *Service) DeleteVpc(
	ctx context.Context,
	op models.Operation,
	state *aws2.DeleteNetworkOperationState,
) error {
	if state.VpcID == "" {
		return nil
	}
	input := &ec2.DeleteVpcInput{VpcId: aws.String(state.VpcID)}
	_, err := s.providers.Dataplane.Ec2.DeleteVpcWithContext(ctx, input)
	if err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != VpcNotFound {
			return xerrors.Errorf("delete vpc: %w", err)
		}
	}
	state.VpcID = ""
	return nil
}
