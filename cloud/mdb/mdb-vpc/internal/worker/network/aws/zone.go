package aws

import (
	"context"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) GetZonesOfRegion(
	ctx context.Context,
	op models.Operation,
	state *aws2.CreateNetworkOperationState,
) error {
	if state.Zones != nil {
		return nil
	}

	input := &ec2.DescribeAvailabilityZonesInput{
		Filters: []*ec2.Filter{{
			Name:   aws.String("region-name"),
			Values: []*string{aws.String(op.Region)},
		}},
	}
	zones, err := s.providers.Dataplane.Ec2.DescribeAvailabilityZonesWithContext(ctx, input)
	if err != nil {
		return xerrors.Errorf("describe AZ: %w", err)
	}
	if len(zones.AvailabilityZones) == 0 {
		return xerrors.Errorf("no AZ in region %q", op.Region)
	}
	state.Zones = make([]string, len(zones.AvailabilityZones))
	for i, az := range zones.AvailabilityZones {
		state.Zones[i] = aws.StringValue(az.ZoneId)
	}
	return nil
}
