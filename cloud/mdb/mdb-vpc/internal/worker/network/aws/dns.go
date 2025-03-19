package aws

import (
	"context"
	"fmt"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/route53"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) AttachDNSZone(
	ctx context.Context,
	op models.Operation,
	state *aws2.CreateNetworkOperationState,
) error {
	if state.HostedZoneID != "" {
		return nil
	}

	zoneID := s.cfg.DNSZoneID
	if zone, ok := s.cfg.DNSRegionalMapping[op.Region]; ok {
		zoneID = zone
	}

	if _, err := s.providers.Controlplane.Route53.CreateVPCAssociationAuthorizationWithContext(
		ctx,
		&route53.CreateVPCAssociationAuthorizationInput{
			HostedZoneId: aws.String(zoneID),
			VPC: &route53.VPC{
				VPCId:     aws.String(state.Vpc.ID),
				VPCRegion: aws.String(op.Region),
			},
		},
	); err != nil {
		return xerrors.Errorf("authorize vpc association: %w", err)
	}

	input := &route53.AssociateVPCWithHostedZoneInput{
		Comment:      aws.String(fmt.Sprintf("operation id %q", op.ID)),
		HostedZoneId: aws.String(zoneID),
		VPC: &route53.VPC{
			VPCId:     aws.String(state.Vpc.ID),
			VPCRegion: aws.String(op.Region),
		},
	}
	_, err := s.providers.Dataplane.Route53.AssociateVPCWithHostedZoneWithContext(ctx, input)
	if err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != DNSZoneAlreadyAttached {
			return xerrors.Errorf("attach dns zone: %w", err)
		}
	}

	ctxlog.Debugf(ctx, s.logger, "Dns zone %q attached to vpc %q", zoneID, state.Vpc.ID)
	state.HostedZoneID = zoneID
	return nil
}

func (s *Service) DetachDNSZone(
	ctx context.Context,
	op models.Operation,
	state *aws2.DeleteNetworkOperationState,
) error {
	var zoneID string
	if state.HostedZoneID != nil {
		zoneID = *state.HostedZoneID
		if zoneID == "" {
			return nil
		}
	} else {
		zoneID = s.cfg.DNSZoneID
		if zone, ok := s.cfg.DNSRegionalMapping[op.Region]; ok {
			zoneID = zone
		}
	}

	input := &route53.DisassociateVPCFromHostedZoneInput{
		Comment:      aws.String(fmt.Sprintf("operation id %q", op.ID)),
		HostedZoneId: aws.String(zoneID),
		VPC: &route53.VPC{
			VPCId:     aws.String(state.VpcID),
			VPCRegion: aws.String(op.Region),
		},
	}
	_, err := s.providers.Dataplane.Route53.DisassociateVPCFromHostedZoneWithContext(ctx, input)
	if err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != DNSZoneAlreadyDetached {
			return xerrors.Errorf("detach dns zone: %w", err)
		}
	}

	ctxlog.Debugf(ctx, s.logger, "Dns zone %q detached from vpc %q", zoneID, state.VpcID)
	state.HostedZoneID = new(string)
	return nil
}
