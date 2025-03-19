package aws

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) CreateNetworkConnection(ctx context.Context, op models.Operation) error {
	params := op.Params.(*models.CreateNetworkConnectionOperationParams)
	state := op.State.(*aws.CreateNetworkConnectionOperationState)

	if err := s.InitCreateNetworkConnectionState(ctx, params, state); err != nil {
		return xerrors.Errorf("init state: %w", err)
	}

	switch state.NetworkConnectionParams.Type {
	case aws.NetworkConnectionPeering:
		if err := s.CreatePeeringConnectionRoutes(ctx, op, state); err != nil {
			return xerrors.Errorf("create peering connection routes: %w", err)
		}

		if !state.IsMarkedPending {
			ncParams := CreateStateToNetworkConnectionParams(state)
			reason := fmt.Sprintf("customer has to accept peering connection %q", state.NetworkConnectionParams.PeeringConnectionID)
			if err := s.db.MarkNetworkConnectionPending(ctx, state.NetworkConnection.ID, reason, ncParams); err != nil {
				return xerrors.Errorf("mark network connection pending: %w", err)
			}
			state.IsMarkedPending = true
		}

		if err := s.CheckPeeringStatus(ctx, op, state); err != nil {
			return xerrors.Errorf("check peering status: %w", err)
		}
	}

	if err := s.db.FinishNetworkConnectionCreating(ctx, state.NetworkConnection.ID); err != nil {
		return xerrors.Errorf("finish network connection creating: %w", err)
	}

	return nil
}

func (s *Service) InitCreateNetworkConnectionState(ctx context.Context, params *models.CreateNetworkConnectionOperationParams, state *aws.CreateNetworkConnectionOperationState) error {
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

	state.IsInited = true
	return nil
}
