package aws

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) DeleteNetworkConnection(ctx context.Context, op models.Operation) error {
	params := op.Params.(*models.DeleteNetworkConnectionOperationParams)
	state := op.State.(*aws.DeleteNetworkConnectionOperationState)

	if err := s.InitDeleteNetworkConnectionState(ctx, params, state); err != nil {
		return xerrors.Errorf("init state: %w", err)
	}

	switch state.NetworkConnectionParams.Type {
	case aws.NetworkConnectionPeering:
		if err := s.DeletePeeringConnectionRoutes(ctx, op, state); err != nil {
			return xerrors.Errorf("delete peering connection routes: %w", err)
		}

		if err := s.DeletePeering(ctx, op, state); err != nil {
			return xerrors.Errorf("delete peering connection: %w", err)
		}
	}

	if err := s.db.DeleteNetworkConnection(ctx, state.NetworkConnection.ID); err != nil {
		return xerrors.Errorf("delete network connection: %w", err)
	}

	return nil
}

func (s *Service) InitDeleteNetworkConnectionState(ctx context.Context, params *models.DeleteNetworkConnectionOperationParams, state *aws.DeleteNetworkConnectionOperationState) error {
	if state.IsInited {
		return nil
	}

	nc, err := s.db.NetworkConnectionByID(ctx, params.NetworkConnectionID)
	if err != nil {
		return xerrors.Errorf("get network connection from db: %w", err)
	}

	net, err := s.db.NetworkByID(ctx, nc.NetworkID)
	if err != nil {
		return xerrors.Errorf("get network from db: %w", err)
	}

	state.NetworkConnection = nc
	state.Network = aws.NetworkToState(net)

	er := net.ExternalResources.(*aws.NetworkExternalResources)
	state.VpcID = er.VpcID

	state.NetworkConnectionParams = nc.Params.(*aws.NetworkConnectionParams)
	state.PeeringID = state.NetworkConnectionParams.PeeringConnectionID

	//state.NetworkConnection = nc
	state.IsInited = true
	return nil
}
