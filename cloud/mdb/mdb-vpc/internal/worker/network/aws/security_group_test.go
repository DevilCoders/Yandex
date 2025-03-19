package aws_test

import (
	"context"
	"testing"

	aws3 "github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/x/github.com/aws/aws-sdk-go/service/ec2/ec2iface/mocks"
	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestAddSecurityGroupRules(t *testing.T) {
	l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	tcs := []struct {
		name  string
		state *aws2.CreateNetworkOperationState
		cfg   config.AwsControlPlaneConfig
	}{
		{
			name: "without outer access",
			state: &aws2.CreateNetworkOperationState{
				Vpc:             &aws2.Vpc{ID: "vpc-id"},
				SecurityGroupID: "sg-id",
			},
			cfg: config.AwsControlPlaneConfig{
				YandexnetsIPv4:       []string{"1.2.3.4", "5.6.7.8"},
				YandexnetsIPv6:       []string{"::1:2:3:4"},
				YandexProjectsNats:   []string{"2.2.2.2", "3.3.3.3"},
				YandexProjectsNatsV6: []string{"::1:1:1:1", "::2:2:2:2"},
				OpenPorts:            []int64{42, 24},
				OpenDataplanePorts:   false,
			},
		},
		{
			name: "open access",
			state: &aws2.CreateNetworkOperationState{
				Vpc:             &aws2.Vpc{ID: "vpc-id"},
				SecurityGroupID: "sg-id",
			},
			cfg: config.AwsControlPlaneConfig{
				YandexnetsIPv4:       []string{"1.2.3.4"},
				YandexnetsIPv6:       []string{"::1:2:3:4", "123::"},
				YandexProjectsNats:   []string{"2.2.2.2", "3.3.3.3"},
				YandexProjectsNatsV6: []string{"::1:1:1:1", "::2:2:2:2"},
				OpenPorts:            []int64{42, 24},
				OpenDataplanePorts:   true,
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			m := mocks.NewMockEC2API(ctrl)
			m.EXPECT().AuthorizeSecurityGroupIngressWithContext(gomock.Any(), gomock.Any()).DoAndReturn(
				func(_ aws3.Context, input *ec2.AuthorizeSecurityGroupIngressInput, _ ...request.Option) (*ec2.AuthorizeSecurityGroupIngressOutput, error) {
					require.Equal(t, tc.state.SecurityGroupID, aws3.StringValue(input.GroupId))
					require.Len(t, input.IpPermissions, 1)
					perms := input.IpPermissions[0]
					require.Len(t, perms.UserIdGroupPairs, 1)
					require.Len(t, perms.IpRanges, len(tc.cfg.YandexnetsIPv4))
					require.Len(t, perms.Ipv6Ranges, len(tc.cfg.YandexnetsIPv6))
					require.Equal(t, int64(0), aws3.Int64Value(perms.FromPort))
					require.Equal(t, int64(0), aws3.Int64Value(perms.ToPort))
					require.Equal(t, "-1", aws3.StringValue(perms.IpProtocol))

					for i, r := range tc.cfg.YandexnetsIPv4 {
						require.Equal(t, r, aws3.StringValue(perms.IpRanges[i].CidrIp))
					}
					for i, r := range tc.cfg.YandexnetsIPv6 {
						require.Equal(t, r, aws3.StringValue(perms.Ipv6Ranges[i].CidrIpv6))
					}

					require.Equal(t, tc.state.SecurityGroupID, aws3.StringValue(perms.UserIdGroupPairs[0].GroupId))
					require.Equal(t, tc.state.Vpc.ID, aws3.StringValue(perms.UserIdGroupPairs[0].VpcId))
					return nil, nil
				}).Times(1)

			for _, port := range tc.cfg.OpenPorts {
				actualPort := aws3.Int64(port)
				m.EXPECT().AuthorizeSecurityGroupIngressWithContext(gomock.Any(), gomock.Any()).DoAndReturn(
					func(_ aws3.Context, input *ec2.AuthorizeSecurityGroupIngressInput, _ ...request.Option) (*ec2.AuthorizeSecurityGroupIngressOutput, error) {
						require.Equal(t, tc.state.SecurityGroupID, aws3.StringValue(input.GroupId))
						require.Len(t, input.IpPermissions, 1)
						perms := input.IpPermissions[0]
						require.Nil(t, perms.UserIdGroupPairs)
						require.Equal(t, actualPort, perms.FromPort)
						require.Equal(t, actualPort, perms.ToPort)
						require.Equal(t, "TCP", aws3.StringValue(perms.IpProtocol))

						if tc.cfg.OpenDataplanePorts {
							require.Len(t, perms.IpRanges, 1)
							require.Len(t, perms.Ipv6Ranges, 1)
							require.Equal(t, "0.0.0.0/0", aws3.StringValue(perms.IpRanges[0].CidrIp))
							require.Equal(t, "::/0", aws3.StringValue(perms.Ipv6Ranges[0].CidrIpv6))
						} else {
							require.Len(t, perms.IpRanges, len(tc.cfg.YandexProjectsNats))
							for i, r := range tc.cfg.YandexProjectsNats {
								require.Equal(t, r, aws3.StringValue(perms.IpRanges[i].CidrIp))
							}
							for i, r := range tc.cfg.YandexProjectsNatsV6 {
								require.Equal(t, r, aws3.StringValue(perms.Ipv6Ranges[i].CidrIpv6))
							}
						}
						return nil, nil
					}).Times(1)
			}

			provider := awsproviders.Provider{Ec2: m}
			providers := awsproviders.Providers{Controlplane: &provider, Dataplane: &provider}
			s := aws.NewCustomService(providers, l, nil, tc.cfg)
			require.NoError(t, s.AddSecurityGroupRules(ctx, models.Operation{}, tc.state))
		})
	}
}
