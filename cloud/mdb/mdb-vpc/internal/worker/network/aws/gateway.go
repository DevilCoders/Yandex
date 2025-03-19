package aws

import (
	"context"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) CreateInternetGateway(
	ctx context.Context,
	op models.Operation,
	state *aws2.CreateNetworkOperationState,
) error {
	if state.IgwID != "" {
		return nil
	}
	createInput := &ec2.CreateInternetGatewayInput{
		TagSpecifications: s.NetworkTags("internet-gateway", op.ProjectID, state.Network.Name, state.Network.ID),
	}
	igw, err := s.providers.Dataplane.Ec2.CreateInternetGatewayWithContext(ctx, createInput)
	if err != nil {
		return xerrors.Errorf("create internet gateway: %w", err)
	}

	attachInput := &ec2.AttachInternetGatewayInput{
		InternetGatewayId: igw.InternetGateway.InternetGatewayId,
		VpcId:             aws.String(state.Vpc.ID),
	}
	_, err = s.providers.Dataplane.Ec2.AttachInternetGatewayWithContext(ctx, attachInput)
	if err != nil {
		return xerrors.Errorf("attach internet gateway: %w", err)
	}

	state.IgwID = aws.StringValue(igw.InternetGateway.InternetGatewayId)
	ctxlog.Debugf(ctx, s.logger, "Created internet gateway: %q", state.IgwID)
	return nil
}

func (s *Service) DeleteInternetGateway(
	ctx context.Context,
	op models.Operation,
	state *aws2.DeleteNetworkOperationState,
) error {
	if state.IgwID == "" {
		return nil
	}
	detachInput := &ec2.DetachInternetGatewayInput{
		InternetGatewayId: aws.String(state.IgwID),
		VpcId:             aws.String(state.VpcID),
	}
	_, err := s.providers.Dataplane.Ec2.DetachInternetGatewayWithContext(ctx, detachInput)
	if err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != IgwAlreadyDetached && awsErr != IgwNotFound {
			return xerrors.Errorf("detach internet gateway: %w", err)
		}
	}

	deleteInput := &ec2.DeleteInternetGatewayInput{InternetGatewayId: aws.String(state.IgwID)}
	_, err = s.providers.Dataplane.Ec2.DeleteInternetGatewayWithContext(ctx, deleteInput)
	if err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != IgwNotFound {
			return xerrors.Errorf("delete internet gateway: %w", err)
		}
	}

	state.IgwID = ""

	return nil
}
