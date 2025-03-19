package aws_test

import (
	"context"
	"testing"

	awssdk "github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/service/route53"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/x/github.com/aws/aws-sdk-go/service/route53/route53iface/mocks"
	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestDetachHostedZone(t *testing.T) {
	l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	region := "region"
	zone := "zone3"
	tcs := []struct {
		name          string
		state         *awsmodels.DeleteNetworkOperationState
		cfg           config.AwsControlPlaneConfig
		validateInput func(*testing.T, *route53.DisassociateVPCFromHostedZoneInput)
		doNotDetach   bool
	}{
		{
			name: "no state, default zone",
			state: &awsmodels.DeleteNetworkOperationState{
				HostedZoneID: nil,
			},
			cfg: config.AwsControlPlaneConfig{
				DNSZoneID:          "zone1",
				DNSRegionalMapping: map[string]string{"asd": "zone2"},
			},
			validateInput: func(t *testing.T, input *route53.DisassociateVPCFromHostedZoneInput) {
				require.Equal(t, "zone1", awssdk.StringValue(input.HostedZoneId))
			},
		},
		{
			name: "no state, zone from cfg",
			state: &awsmodels.DeleteNetworkOperationState{
				HostedZoneID: nil,
			},
			cfg: config.AwsControlPlaneConfig{
				DNSZoneID:          "zone1",
				DNSRegionalMapping: map[string]string{region: "zone2"},
			},
			validateInput: func(t *testing.T, input *route53.DisassociateVPCFromHostedZoneInput) {
				require.Equal(t, "zone2", awssdk.StringValue(input.HostedZoneId))
			},
		},
		{
			name: "zone from state",
			state: &awsmodels.DeleteNetworkOperationState{
				HostedZoneID: &zone,
			},
			cfg: config.AwsControlPlaneConfig{
				DNSZoneID:          "zone1",
				DNSRegionalMapping: map[string]string{region: "zone2"},
			},
			validateInput: func(t *testing.T, input *route53.DisassociateVPCFromHostedZoneInput) {
				require.Equal(t, zone, awssdk.StringValue(input.HostedZoneId))
			},
		},
		{
			name: "do not detach",
			state: &awsmodels.DeleteNetworkOperationState{
				HostedZoneID: new(string),
			},
			cfg: config.AwsControlPlaneConfig{
				DNSZoneID:          "zone1",
				DNSRegionalMapping: map[string]string{region: "zone2"},
			},
			doNotDetach: true,
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			m := mocks.NewMockRoute53API(ctrl)
			if !tc.doNotDetach {
				m.EXPECT().DisassociateVPCFromHostedZoneWithContext(gomock.Any(), gomock.Any()).Times(1).DoAndReturn(
					func(_ awssdk.Context, input *route53.DisassociateVPCFromHostedZoneInput, _ ...request.Option) (*route53.DisassociateVPCFromHostedZoneOutput, error) {
						tc.validateInput(t, input)
						return nil, nil
					})
			}
			provider := awsproviders.Provider{Route53: m}
			providers := awsproviders.Providers{Controlplane: &provider, Dataplane: &provider}
			s := aws.NewCustomService(providers, l, nil, tc.cfg)
			require.NoError(t, s.DetachDNSZone(ctx, models.Operation{Region: region}, tc.state))
			require.Equal(t, "", *tc.state.HostedZoneID)
		})
	}
}
