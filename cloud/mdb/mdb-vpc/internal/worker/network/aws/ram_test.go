package aws_test

import (
	"context"
	"testing"

	awssdk "github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ram"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/x/github.com/aws/aws-sdk-go/service/ram/ramiface/mocks"
	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	vpcdbmocks "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestShareTGW(t *testing.T) {
	l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	region := "region"
	account := "account"
	share := "share"
	shareInv := "share inv"

	ctx := context.Background()
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	cpM := mocks.NewMockRAMAPI(ctrl)
	cpM.EXPECT().AssociateResourceShareWithContext(gomock.Any(), &ram.AssociateResourceShareInput{
		Principals:       awssdk.StringSlice([]string{account}),
		ResourceShareArn: awssdk.String(share),
	}).Return(nil, nil)

	dpM := mocks.NewMockRAMAPI(ctrl)
	dpM.EXPECT().GetResourceShareInvitationsWithContext(gomock.Any(), &ram.GetResourceShareInvitationsInput{
		ResourceShareArns: awssdk.StringSlice([]string{share}),
	}).Return(&ram.GetResourceShareInvitationsOutput{
		ResourceShareInvitations: []*ram.ResourceShareInvitation{{
			ResourceShareInvitationArn: awssdk.String(shareInv),
		}},
	}, nil)
	dpM.EXPECT().AcceptResourceShareInvitationWithContext(gomock.Any(), &ram.AcceptResourceShareInvitationInput{
		ResourceShareInvitationArn: awssdk.String(shareInv),
	}).Return(nil, nil)

	cpP := awsproviders.Provider{RAM: cpM}
	dpP := awsproviders.Provider{RAM: dpM}
	providers := awsproviders.Providers{Controlplane: &cpP, Dataplane: &dpP}

	state := &awsmodels.ImportVPCOperationState{
		AccountID: account,
	}

	s := aws.NewCustomService(providers, l, nil, config.AwsControlPlaneConfig{
		TransitGateways: map[string]config.TransitGateway{region: {ShareARN: share}},
	})
	require.NoError(t, s.ShareTGW(ctx, models.Operation{Region: region}, state))
	require.True(t, state.TgwShared)
}

func TestUnshareTGW(t *testing.T) {
	l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	region := "region"
	account := "account"
	share := "share"
	project := "project"

	tcs := []struct {
		name                  string
		mockCP                func(m *mocks.MockRAMAPI)
		importedNetworksCount int
	}{
		{
			name: "last imported network in region",
			mockCP: func(m *mocks.MockRAMAPI) {
				m.EXPECT().DisassociateResourceShareWithContext(gomock.Any(), &ram.DisassociateResourceShareInput{
					Principals:       awssdk.StringSlice([]string{account}),
					ResourceShareArn: awssdk.String(share),
				}).Return(nil, nil)
			},
		},
		{
			name:                  "one more network, nothing to unshare",
			mockCP:                func(m *mocks.MockRAMAPI) {},
			importedNetworksCount: 1,
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()

			cpM := mocks.NewMockRAMAPI(ctrl)
			tc.mockCP(cpM)

			db := vpcdbmocks.NewMockVPCDB(ctrl)
			db.EXPECT().ImportedNetworks(gomock.Any(), project, region, models.ProviderAWS).Return(make([]models.Network, tc.importedNetworksCount), nil)

			cpP := awsproviders.Provider{RAM: cpM}
			dpP := awsproviders.Provider{RAM: nil}
			providers := awsproviders.Providers{Controlplane: &cpP, Dataplane: &dpP}

			state := &awsmodels.DeleteNetworkOperationState{
				AccountID: optional.NewString(account),
			}

			s := aws.NewCustomService(providers, l, db, config.AwsControlPlaneConfig{
				TransitGateways: map[string]config.TransitGateway{region: {ShareARN: share}},
			})
			require.NoError(t, s.UnshareTGW(ctx, models.Operation{Region: region, Provider: models.ProviderAWS, ProjectID: project}, state))
			require.True(t, state.TgwUnshared)
		})
	}
}
