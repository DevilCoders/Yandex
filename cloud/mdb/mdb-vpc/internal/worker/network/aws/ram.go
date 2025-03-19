package aws

import (
	"context"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ram"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) ShareTGW(ctx context.Context, op models.Operation, state *awsmodels.ImportVPCOperationState) error {
	if state.TgwShared {
		return nil
	}

	tgw, ok := s.cfg.TransitGateways[op.Region]
	if !ok {
		return xerrors.Errorf("there are no known transit gateways in region %v", op.Region)
	}

	if _, err := s.providers.Controlplane.RAM.AssociateResourceShareWithContext(ctx, &ram.AssociateResourceShareInput{
		Principals:       aws.StringSlice([]string{state.AccountID}),
		ResourceShareArn: aws.String(tgw.ShareARN),
	}); err != nil {
		return xerrors.Errorf("associate resource share: %w", err)
	}

	data, err := s.providers.Dataplane.RAM.GetResourceShareInvitationsWithContext(ctx, &ram.GetResourceShareInvitationsInput{
		ResourceShareArns: aws.StringSlice([]string{tgw.ShareARN}),
	})
	if err != nil {
		return xerrors.Errorf("get resource share invitation: %w", err)
	}
	if len(data.ResourceShareInvitations) != 1 {
		return xerrors.Errorf("assumed that there will be exactly one invitation, got %v", data.ResourceShareInvitations)
	}

	if _, err = s.providers.Dataplane.RAM.AcceptResourceShareInvitationWithContext(ctx, &ram.AcceptResourceShareInvitationInput{
		ResourceShareInvitationArn: data.ResourceShareInvitations[0].ResourceShareInvitationArn,
	}); err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != ShareAlreadyAccepted {
			return xerrors.Errorf("associate resource share: %w", err)
		}
	}

	state.TgwShared = true

	return nil
}

func (s *Service) UnshareTGW(ctx context.Context, op models.Operation, state *awsmodels.DeleteNetworkOperationState) error {
	// TODO unshare only it is last imported network in region for project
	if state.TgwUnshared {
		return nil
	}

	importedNets, err := s.db.ImportedNetworks(ctx, op.ProjectID, op.Region, op.Provider)
	if err != nil {
		return xerrors.Errorf("get imported networks: %w", err)
	}

	if len(importedNets) > 0 {
		s.logger.Debugf("there are %d more imported networks in this region, will not do tgw unshare", len(importedNets))
		state.TgwUnshared = true
		return nil
	}

	tgw, ok := s.cfg.TransitGateways[op.Region]
	if !ok {
		return xerrors.Errorf("there are no known transit gateways in region %v", op.Region)
	}

	accountID, err := state.AccountID.Get()
	if err != nil {
		return xerrors.Errorf("can not get accountID from state, it is a bug")
	}

	if _, err := s.providers.Controlplane.RAM.DisassociateResourceShareWithContext(ctx, &ram.DisassociateResourceShareInput{
		Principals:       aws.StringSlice([]string{accountID}),
		ResourceShareArn: aws.String(tgw.ShareARN),
	}); err != nil {
		return xerrors.Errorf("disassociate resource share: %w", err)
	}

	state.TgwUnshared = true
	return nil
}
