package aws

import (
	"context"

	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws/ready"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) ImportVPC(ctx context.Context, op models.Operation) error {
	params := op.Params.(*aws.ImportVPCOperationParams)
	state := op.State.(*aws.ImportVPCOperationState)

	dpProvider := s.providers.AssumedDataplane(params.IamRoleArn)
	providers := awsproviders.Providers{
		Controlplane: s.providers.Controlplane,
		Dataplane:    dpProvider,
	}

	srv := NewCustomService(providers, s.logger, s.db, s.cfg)

	if err := srv.InitImportVPCState(ctx, params, state); err != nil {
		return xerrors.Errorf("init state: %w", err)
	}

	if err := srv.ShareTGW(ctx, op, state); err != nil {
		return xerrors.Errorf("share tgw: %w", err)
	}

	if err := srv.createVPCStuff(ctx, op, &state.CreateNetworkOperationState); err != nil {
		return err
	}

	externalResources := ImportVPCStateToExternalResources(state)
	if err := s.db.FinishNetworkCreating(ctx, state.Network.ID, state.Vpc.IPv6CIDR, externalResources); err != nil {
		return xerrors.Errorf("finish network creating: %w", err)
	}

	return nil
}

func (s *Service) InitImportVPCState(ctx context.Context, params *aws.ImportVPCOperationParams, state *aws.ImportVPCOperationState) error {
	if state.IsInited {
		return nil
	}

	net, err := s.db.NetworkByID(ctx, params.NetworkID)
	if err != nil {
		return xerrors.Errorf("get network from db: %w", err)
	}
	state.Network = aws.NetworkToState(net)

	vpc, err := ready.WaitForVPCReady(ctx, s.GetVpc, IsNotAvailable, params.VpcID, s.logger)
	if err != nil {
		return xerrors.Errorf("get vpc: %w", err)
	}
	state.Vpc = &vpc

	state.AccountID = params.AccountID
	state.IamRoleArn = params.IamRoleArn
	state.IsInited = true
	return nil
}
