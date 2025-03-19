package aws

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) CreateNetwork(ctx context.Context, op models.Operation) error {
	params := op.Params.(*models.CreateNetworkOperationParams)
	state := op.State.(*aws.CreateNetworkOperationState)

	if err := s.InitCreateNetworkState(ctx, params, state); err != nil {
		return xerrors.Errorf("init state: %w", err)
	}

	if err := s.CreateVpc(ctx, op, state); err != nil {
		return xerrors.Errorf("create vpc: %w", err)
	}

	if err := s.createVPCStuff(ctx, op, state); err != nil {
		return err
	}

	externalResources := CreateStateToExternalResources(state)
	if err := s.db.FinishNetworkCreating(ctx, state.Network.ID, state.Vpc.IPv6CIDR, externalResources); err != nil {
		return xerrors.Errorf("finish network creating: %w", err)
	}

	return nil
}

func (s *Service) createVPCStuff(ctx context.Context, op models.Operation, state *aws.CreateNetworkOperationState) error {
	if err := s.TuneVpc(ctx, op, state); err != nil {
		return xerrors.Errorf("tune vpc: %w", err)
	}

	if err := s.AttachDNSZone(ctx, op, state); err != nil {
		return xerrors.Errorf("attach dns zone: %w", err)
	}

	if err := s.GetZonesOfRegion(ctx, op, state); err != nil {
		return xerrors.Errorf("get zones: %w", err)
	}

	if err := s.CreateSubnets(ctx, op, state); err != nil {
		return xerrors.Errorf("create subnets: %w", err)
	}

	if err := s.CreateTransitGatewayAttachment(ctx, op, state); err != nil {
		return xerrors.Errorf("create transit gateway attachment: %w", err)
	}

	if err := s.CreateInternetGateway(ctx, op, state); err != nil {
		return xerrors.Errorf("create igw: %w", err)
	}

	if err := s.CreateRoutes(ctx, op, state); err != nil {
		return xerrors.Errorf("create routes: %w", err)
	}

	if err := s.CreateSecurityGroup(ctx, op, state); err != nil {
		return xerrors.Errorf("create security group: %w", err)
	}

	if err := s.AddSecurityGroupRules(ctx, op, state); err != nil {
		return xerrors.Errorf("add security group rules: %w", err)
	}

	return nil
}

func (s *Service) InitCreateNetworkState(ctx context.Context, params *models.CreateNetworkOperationParams, state *aws.CreateNetworkOperationState) error {
	if state.IsInited {
		return nil
	}

	net, err := s.db.NetworkByID(ctx, params.NetworkID)
	if err != nil {
		return xerrors.Errorf("get network from db: %w", err)
	}

	state.Network = aws.NetworkToState(net)
	state.IsInited = true
	return nil
}
