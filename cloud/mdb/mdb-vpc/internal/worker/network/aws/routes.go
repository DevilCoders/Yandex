package aws

import (
	"context"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	IPv4Egress = aws.String("0.0.0.0/0")
	IPv6Egress = aws.String("::/0")
)

func (s *Service) CreateRoutes(ctx context.Context, op models.Operation, state *aws2.CreateNetworkOperationState) error {
	if !state.EgressRoutesCreated {
		if state.VpcRouteTableID == "" {
			tableID, err := s.getRouteTableID(ctx, "vpc-id", state.Vpc.ID)
			if err != nil {
				return xerrors.Errorf("get route table ID: %w", err)
			}
			state.VpcRouteTableID = tableID
		}

		v4EgressInput := &ec2.CreateRouteInput{
			RouteTableId:         aws.String(state.VpcRouteTableID),
			DestinationCidrBlock: IPv4Egress,
			GatewayId:            aws.String(state.IgwID),
		}
		_, err := s.providers.Dataplane.Ec2.CreateRouteWithContext(ctx, v4EgressInput)
		if err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != RouteAlreadyExists {
				return xerrors.Errorf("create v4 internet gateway route: %w", err)
			}
		}

		v6EgressInput := &ec2.CreateRouteInput{
			RouteTableId:             aws.String(state.VpcRouteTableID),
			DestinationIpv6CidrBlock: IPv6Egress,
			GatewayId:                aws.String(state.IgwID),
		}
		_, err = s.providers.Dataplane.Ec2.CreateRouteWithContext(ctx, v6EgressInput)
		if err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != RouteAlreadyExists {
				return xerrors.Errorf("create v6 internet gateway route: %w", err)
			}
		}

		state.EgressRoutesCreated = true
	}

	if !state.NlbRoutesCreated {
		for _, nlbNet := range s.cfg.NlbNets {
			toNlbInput := &ec2.CreateRouteInput{
				RouteTableId:             aws.String(state.VpcRouteTableID),
				DestinationIpv6CidrBlock: aws.String(nlbNet),
				TransitGatewayId:         aws.String(s.cfg.TransitGateways[op.Region].ID),
			}
			_, err := s.providers.Dataplane.Ec2.CreateRouteWithContext(ctx, toNlbInput)
			if err != nil {
				if awsErr, _ := getAwsErrorCode(err); awsErr != RouteAlreadyExists {
					return xerrors.Errorf("create route to NLB: %w", err)
				}
			}
		}
		state.NlbRoutesCreated = true
	}

	return nil
}

func (s *Service) CreatePeeringConnectionRoutes(ctx context.Context, _ models.Operation, state *aws2.CreateNetworkConnectionOperationState) error {
	if state.PeeringRoutesCreated {
		return nil
	}

	if state.VpcRouteTableID == "" {
		tableID, err := s.getRouteTableID(ctx, "vpc-id", state.VpcID)
		if err != nil {
			return xerrors.Errorf("get route table ID: %w", err)
		}
		state.VpcRouteTableID = tableID
	}

	v4Input := &ec2.CreateRouteInput{
		RouteTableId:           aws.String(state.VpcRouteTableID),
		DestinationCidrBlock:   aws.String(state.NetworkConnectionParams.IPv4),
		VpcPeeringConnectionId: aws.String(state.NetworkConnectionParams.PeeringConnectionID),
	}
	_, err := s.providers.Dataplane.Ec2.CreateRouteWithContext(ctx, v4Input)
	if err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != RouteAlreadyExists {
			return xerrors.Errorf("create v4 peering connection route: %w", err)
		}
	}

	if state.NetworkConnectionParams.IPv6 != "" {
		v6Input := &ec2.CreateRouteInput{
			RouteTableId:             aws.String(state.VpcRouteTableID),
			DestinationIpv6CidrBlock: aws.String(state.NetworkConnectionParams.IPv6),
			VpcPeeringConnectionId:   aws.String(state.NetworkConnectionParams.PeeringConnectionID),
		}
		_, err = s.providers.Dataplane.Ec2.CreateRouteWithContext(ctx, v6Input)
		if err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != RouteAlreadyExists {
				return xerrors.Errorf("create v6 peering connection route: %w", err)
			}
		}
	}
	state.PeeringRoutesCreated = true
	return nil
}

func (s *Service) DeleteRoutes(ctx context.Context, op models.Operation, state *aws2.DeleteNetworkOperationState) error {
	if state.AreRoutesDeleted {
		return nil
	}
	if state.VpcRouteTableID == "" {
		tableID, err := s.getRouteTableID(ctx, "vpc-id", state.VpcID)
		if err != nil {
			return xerrors.Errorf("get route table ID: %w", err)
		}
		state.VpcRouteTableID = tableID
	}

	v4Input := &ec2.DeleteRouteInput{
		RouteTableId:         aws.String(state.VpcRouteTableID),
		DestinationCidrBlock: IPv4Egress,
	}
	if err := s.deleteRoute(ctx, v4Input); err != nil {
		return xerrors.Errorf("delete v4 egress route: %w", err)
	}

	v6Input := &ec2.DeleteRouteInput{
		RouteTableId:             aws.String(state.VpcRouteTableID),
		DestinationIpv6CidrBlock: IPv6Egress,
	}
	if err := s.deleteRoute(ctx, v6Input); err != nil {
		return xerrors.Errorf("delete v6 egress route: %w", err)
	}

	for _, nlbNet := range s.cfg.NlbNets {
		toNlbInput := &ec2.DeleteRouteInput{
			RouteTableId:             aws.String(state.VpcRouteTableID),
			DestinationIpv6CidrBlock: aws.String(nlbNet),
		}
		if err := s.deleteRoute(ctx, toNlbInput); err != nil {
			return xerrors.Errorf("delete route to NLB: %w", err)
		}
	}

	state.AreRoutesDeleted = true
	return nil
}

func (s *Service) DeletePeeringConnectionRoutes(ctx context.Context, _ models.Operation, state *aws2.DeleteNetworkConnectionOperationState) error {
	if state.PeeringRoutesDeleted {
		return nil
	}

	if state.VpcRouteTableID == "" {
		tableID, err := s.getRouteTableID(ctx, "vpc-id", state.VpcID)
		if err != nil {
			return xerrors.Errorf("get route table ID: %w", err)
		}
		state.VpcRouteTableID = tableID
	}

	v4Input := &ec2.DeleteRouteInput{
		RouteTableId:         aws.String(state.VpcRouteTableID),
		DestinationCidrBlock: aws.String(state.NetworkConnectionParams.IPv4),
	}
	if err := s.deleteRoute(ctx, v4Input); err != nil {
		return xerrors.Errorf("delete v4 peering connection route: %w", err)
	}

	if state.NetworkConnectionParams.IPv6 != "" {
		v6Input := &ec2.DeleteRouteInput{
			RouteTableId:             aws.String(state.VpcRouteTableID),
			DestinationIpv6CidrBlock: aws.String(state.NetworkConnectionParams.IPv6),
		}
		if err := s.deleteRoute(ctx, v6Input); err != nil {
			return xerrors.Errorf("delete v6 peering connection route: %w", err)
		}
	}

	state.PeeringRoutesDeleted = true
	return nil
}

func (s *Service) deleteRoute(ctx context.Context, input *ec2.DeleteRouteInput) error {
	_, err := s.providers.Dataplane.Ec2.DeleteRouteWithContext(ctx, input)
	if err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != RouteNotFound {
			return err
		}
	}
	return nil
}

func (s *Service) getRouteTableID(ctx context.Context, resType string, resID string) (string, error) {
	vpcRouteTableInput := &ec2.DescribeRouteTablesInput{
		Filters: []*ec2.Filter{{
			Name:   aws.String(resType),
			Values: aws.StringSlice([]string{resID}),
		}},
	}
	routeTable, err := s.providers.Dataplane.Ec2.DescribeRouteTablesWithContext(ctx, vpcRouteTableInput)
	if err != nil {
		return "", xerrors.Errorf("describe vpc route table: %w", err)
	}

	if len(routeTable.RouteTables) == 0 {
		return "", xerrors.Errorf("can not find route table for %s %q", resType, resID)
	}

	return aws.StringValue(routeTable.RouteTables[0].RouteTableId), nil
}
