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

func (s *Service) CreateTransitGatewayAttachment(ctx context.Context, op models.Operation, state *aws2.CreateNetworkOperationState) error {
	tgw, ok := s.cfg.TransitGateways[op.Region]
	if !ok {
		return xerrors.Errorf("there are no known transit gateways in region %v", op.Region)
	}

	if state.TgwAttachmentID == "" {
		subnets := make([]*string, len(state.Subnets))
		for i, subnet := range state.Subnets {
			subnets[i] = aws.String(subnet.ID)
		}

		createInput := &ec2.CreateTransitGatewayVpcAttachmentInput{
			Options: &ec2.CreateTransitGatewayVpcAttachmentRequestOptions{
				DnsSupport:  aws.String("enable"),
				Ipv6Support: aws.String("enable"),
			},
			SubnetIds:         subnets,
			TagSpecifications: s.NetworkTags("transit-gateway-attachment", op.ProjectID, state.Network.Name, state.Network.ID),
			TransitGatewayId:  aws.String(tgw.ID),
			VpcId:             aws.String(state.Vpc.ID),
		}
		createRes, err := s.providers.Dataplane.Ec2.CreateTransitGatewayVpcAttachmentWithContext(ctx, createInput)
		if err != nil {
			return xerrors.Errorf("attach transit gateway: %w", err)
		}
		state.TgwAttachmentID = aws.StringValue(createRes.TransitGatewayVpcAttachment.TransitGatewayAttachmentId)
	}

	if err := ready.WaitForTgwAttachmentReady(ctx, s.GetTgwAttachment, IsNotAvailable, state.TgwAttachmentID, s.logger); err != nil {
		return xerrors.Errorf("wait for tgw attachment ready: %w", err)
	}

	if !state.TgwAssociated {
		associateInput := &ec2.AssociateTransitGatewayRouteTableInput{
			TransitGatewayAttachmentId: aws.String(state.TgwAttachmentID),
			TransitGatewayRouteTableId: aws.String(tgw.RouteTableID),
		}
		_, err := s.providers.Controlplane.Ec2.AssociateTransitGatewayRouteTableWithContext(ctx, associateInput)
		if err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != TgwRouteTableAlreadyAssociated {
				return xerrors.Errorf("associate tgw route table: %w", err)
			}
		}

		if err := ready.WaitForTgwRouteTableAssociationReady(
			ctx, s.GetTgwRouteTableAssociation, IsNotAvailable, tgw.RouteTableID, state.Vpc.ID, s.logger); err != nil {
			return xerrors.Errorf("wait for tgw route table association ready: %w", err)
		}
		state.TgwAssociated = true
	}

	if !state.TgwPropEnabled {
		if _, err := s.providers.Controlplane.Ec2.EnableTransitGatewayRouteTablePropagationWithContext(
			ctx, &ec2.EnableTransitGatewayRouteTablePropagationInput{
				TransitGatewayAttachmentId: aws.String(state.TgwAttachmentID),
				TransitGatewayRouteTableId: aws.String(tgw.RouteTableID),
			},
		); err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != TgwRouteTablePropagationAlreadyEnabled {
				return xerrors.Errorf("enable tgw route table propagation: %w", err)
			}
		}
		state.TgwPropEnabled = true
	}

	return nil
}

func (s *Service) GetTgwAttachment(ctx context.Context, id string) error {
	res, err := s.providers.Dataplane.Ec2.DescribeTransitGatewayVpcAttachmentsWithContext(ctx, &ec2.DescribeTransitGatewayVpcAttachmentsInput{
		TransitGatewayAttachmentIds: aws.StringSlice([]string{id}),
	})
	if err != nil {
		return xerrors.Errorf("describe tgw attachments: %w", err)
	}
	if len(res.TransitGatewayVpcAttachments) == 0 {
		return semerr.NotFound("tgw attachment doesn't exist")
	}
	if aws.StringValue(res.TransitGatewayVpcAttachments[0].State) != "available" {
		return IsNotAvailable
	}

	return nil
}

func (s *Service) GetTgwRouteTableAssociation(ctx context.Context, routeTableID string, vpcID string) error {
	res, err := s.providers.Controlplane.Ec2.GetTransitGatewayRouteTableAssociationsWithContext(ctx, &ec2.GetTransitGatewayRouteTableAssociationsInput{
		Filters: []*ec2.Filter{
			{
				Name:   aws.String("resource-id"),
				Values: aws.StringSlice([]string{vpcID}),
			},
			{
				Name:   aws.String("resource-type"),
				Values: aws.StringSlice([]string{"vpc"}),
			},
		},
		TransitGatewayRouteTableId: aws.String(routeTableID),
	})
	if err != nil {
		return xerrors.Errorf("get tgw route table associations: %w", err)
	}
	if len(res.Associations) == 0 {
		return semerr.NotFound("tgw route table association doesn't exist")
	}
	if aws.StringValue(res.Associations[0].State) != "associated" {
		return IsNotAvailable
	}

	return nil
}

func (s *Service) DeleteTransitGatewayAttachment(ctx context.Context, op models.Operation, state *aws2.DeleteNetworkOperationState) error {
	if state.TgwAttachmentID == "" {
		return nil
	}

	if _, err := s.providers.Dataplane.Ec2.DeleteTransitGatewayVpcAttachmentWithContext(ctx, &ec2.DeleteTransitGatewayVpcAttachmentInput{
		TransitGatewayAttachmentId: aws.String(state.TgwAttachmentID),
	}); err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != TgwAttachmentNotFound {
			return xerrors.Errorf("delete tgw attachment: %w", err)
		}
	}
	state.TgwAttachmentID = ""
	return nil
}
