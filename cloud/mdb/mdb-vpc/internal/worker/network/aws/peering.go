package aws

import (
	"context"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) CheckPeeringStatus(ctx context.Context, op models.Operation, state *awsmodels.CreateNetworkConnectionOperationState) error {
	res, err := s.providers.Dataplane.Ec2.DescribeVpcPeeringConnectionsWithContext(ctx, &ec2.DescribeVpcPeeringConnectionsInput{
		VpcPeeringConnectionIds: aws.StringSlice([]string{state.NetworkConnectionParams.PeeringConnectionID}),
	})
	if err != nil {
		return xerrors.Errorf("get peering connection: %w", err)
	}
	if len(res.VpcPeeringConnections) != 1 {
		return xerrors.Errorf("got not 1 peering connection: %v. Probably, this is the AWS bug.", res)
	}

	if aws.StringValue(res.VpcPeeringConnections[0].Status.Code) != ec2.VpcPeeringConnectionStateReasonCodeActive {
		return xerrors.Errorf("peering connection in not active yet, got %q", aws.StringValue(res.VpcPeeringConnections[0].Status.Code))
	}

	return nil
}

func (s *Service) DeletePeering(
	ctx context.Context, _ models.Operation, state *awsmodels.DeleteNetworkConnectionOperationState) error {
	if state.PeeringID == "" {
		return nil
	}

	input := &ec2.DeleteVpcPeeringConnectionInput{
		VpcPeeringConnectionId: aws.String(state.PeeringID),
	}
	_, err := s.providers.Dataplane.Ec2.DeleteVpcPeeringConnectionWithContext(ctx, input)
	if err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != PeeringNotFound {
			return xerrors.Errorf("delete peering connection: %w", err)
		}
	}
	state.PeeringID = ""

	return nil
}
