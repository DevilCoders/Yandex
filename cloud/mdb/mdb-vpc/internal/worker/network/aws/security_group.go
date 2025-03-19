package aws

import (
	"context"
	"fmt"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) CreateSecurityGroup(
	ctx context.Context,
	op models.Operation,
	state *aws2.CreateNetworkOperationState,
) error {
	if state.SecurityGroupID != "" {
		return nil
	}

	name := fmt.Sprintf("%s: %s", op.ProjectID, state.Network.Name)
	input := &ec2.CreateSecurityGroupInput{
		GroupName:         aws.String(name),
		Description:       aws.String(name),
		TagSpecifications: s.NetworkTags("security-group", op.ProjectID, state.Network.Name, state.Network.ID),
		VpcId:             aws.String(state.Vpc.ID),
	}
	res, err := s.providers.Dataplane.Ec2.CreateSecurityGroupWithContext(ctx, input)
	if err != nil {
		return xerrors.Errorf("create sg: %w", err)
	}

	state.SecurityGroupID = aws.StringValue(res.GroupId)

	return nil
}

func (s *Service) AddSecurityGroupRules(
	ctx context.Context,
	op models.Operation,
	state *aws2.CreateNetworkOperationState,
) error {
	if _, err := s.providers.Dataplane.Ec2.AuthorizeSecurityGroupIngressWithContext(ctx, &ec2.AuthorizeSecurityGroupIngressInput{
		GroupId: aws.String(state.SecurityGroupID),
		IpPermissions: []*ec2.IpPermission{{
			FromPort:   aws.Int64(0),
			IpRanges:   ipv4RangesToAws(s.cfg.YandexnetsIPv4, "access from _YANDEXNETS_"),
			Ipv6Ranges: ipv6RangesToAws(s.cfg.YandexnetsIPv6, "access from _YANDEXNETS_"),
			IpProtocol: aws.String("-1"),
			ToPort:     aws.Int64(0),
			UserIdGroupPairs: []*ec2.UserIdGroupPair{{
				GroupId: aws.String(state.SecurityGroupID),
				VpcId:   aws.String(state.Vpc.ID),
			}},
		}},
	}); err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != SecurityGroupRuleAlreadyExists {
			return xerrors.Errorf("add YANDEXNETS access: %w", err)
		}
	}

	var ipRanges []*ec2.IpRange
	var ipv6Ranges []*ec2.Ipv6Range
	if s.cfg.OpenDataplanePorts {
		ipRanges = ipv4RangesToAws([]string{"0.0.0.0/0"}, "access from the Internet")
		ipv6Ranges = ipv6RangesToAws([]string{"::/0"}, "access from the Internet")
	} else {
		ipRanges = ipv4RangesToAws(s.cfg.YandexProjectsNats, "access from yandex projects")
		ipv6Ranges = ipv6RangesToAws(s.cfg.YandexProjectsNatsV6, "access from yandex projects")
	}

	for _, port := range s.cfg.OpenPorts {
		if _, err := s.providers.Dataplane.Ec2.AuthorizeSecurityGroupIngressWithContext(ctx, &ec2.AuthorizeSecurityGroupIngressInput{
			GroupId: aws.String(state.SecurityGroupID),
			IpPermissions: []*ec2.IpPermission{{
				FromPort:   aws.Int64(port),
				IpRanges:   ipRanges,
				Ipv6Ranges: ipv6Ranges,
				IpProtocol: aws.String("TCP"),
				ToPort:     aws.Int64(port),
			}},
		}); err != nil {
			if awsErr, _ := getAwsErrorCode(err); awsErr != SecurityGroupRuleAlreadyExists {
				return xerrors.Errorf("add yandex projects access by port %d: %w", port, err)
			}
		}
	}

	return nil
}

func (s *Service) DeleteSecurityGroup(
	ctx context.Context,
	op models.Operation,
	state *aws2.DeleteNetworkOperationState,
) error {
	if state.SecurityGroupID == "" {
		return nil
	}

	input := &ec2.DeleteSecurityGroupInput{
		GroupId: aws.String(state.SecurityGroupID),
	}
	if _, err := s.providers.Dataplane.Ec2.DeleteSecurityGroupWithContext(ctx, input); err != nil {
		if awsErr, _ := getAwsErrorCode(err); awsErr != SecurityGroupNotFound {
			return xerrors.Errorf("delete security group: %w", err)
		}
	}

	state.SecurityGroupID = ""

	return nil
}
