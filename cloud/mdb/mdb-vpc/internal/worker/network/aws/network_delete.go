package aws

import (
	"context"

	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) DeleteNetwork(ctx context.Context, op models.Operation) error {
	params := op.Params.(*models.DeleteNetworkOperationParams)
	state := op.State.(*aws.DeleteNetworkOperationState)

	if err := s.InitDeleteNetworkState(ctx, params, state); err != nil {
		return xerrors.Errorf("init state: %w", err)
	}

	if state.IamRoleArn.Valid {
		dpProvider := s.providers.AssumedDataplane(state.IamRoleArn.String)
		providers := awsproviders.Providers{
			Controlplane: s.providers.Controlplane,
			Dataplane:    dpProvider,
		}

		srv := NewCustomService(providers, s.logger, s.db, s.cfg)

		if err := srv.deleteVPCStuff(ctx, op, state); err != nil {
			return err
		}

		if err := srv.UnshareTGW(ctx, op, state); err != nil {
			return xerrors.Errorf("unshare tgw: %w", err)
		}
	} else {
		if err := s.deleteVPCStuff(ctx, op, state); err != nil {
			return err
		}

		if err := s.DeleteVpc(ctx, op, state); err != nil {
			return xerrors.Errorf("delete vpc: %w", err)
		}
	}

	if err := s.db.DeleteNetwork(ctx, params.NetworkID); err != nil {
		return xerrors.Errorf("delete network: %w", err)
	}

	return nil
}

func (s *Service) deleteVPCStuff(ctx context.Context, op models.Operation, state *aws.DeleteNetworkOperationState) error {
	if err := s.DetachDNSZone(ctx, op, state); err != nil {
		return xerrors.Errorf("detach dns zone: %w", err)
	}

	if err := s.DeleteRoutes(ctx, op, state); err != nil {
		return xerrors.Errorf("delete routes: %w", err)
	}

	if err := s.DeleteInternetGateway(ctx, op, state); err != nil {
		return xerrors.Errorf("delete gateway: %w", err)
	}

	if err := s.DeleteTransitGatewayAttachment(ctx, op, state); err != nil {
		return xerrors.Errorf("delete transit gateway attachment: %w", err)
	}

	if err := s.DeleteSubnets(ctx, op, state); err != nil {
		return xerrors.Errorf("delete subnets: %w", err)
	}

	if err := s.DeleteSecurityGroup(ctx, op, state); err != nil {
		return xerrors.Errorf("delete security group: %w", err)
	}

	return nil
}

func (s *Service) InitDeleteNetworkState(ctx context.Context, params *models.DeleteNetworkOperationParams, state *aws.DeleteNetworkOperationState) error {
	if state.IsInited {
		return nil
	}

	net, err := s.db.NetworkByID(ctx, params.NetworkID)
	if err != nil {
		return xerrors.Errorf("get network from db: %w", err)
	}

	ExternalResourcesToDeleteState(net.ExternalResources, state)

	state.IsInited = true
	return nil
}
