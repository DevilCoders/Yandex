package aws_test

import (
	"context"
	"testing"

	awslib "github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	ec2mocks "a.yandex-team.ru/cloud/mdb/internal/x/github.com/aws/aws-sdk-go/service/ec2/ec2iface/mocks"
	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	vpcdbmocks "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var (
	createNetworkConnectionConsts = struct {
		region          string
		project         string
		ncID            string
		netID           string
		peerAccID       string
		peerVpcID       string
		peerIpv4        string
		peerIpv6        string
		peerRegion      string
		netName         string
		managedNetIpv4  string
		managedNetIpv6  string
		managedNetVpcID string
		peeringID       string
		routeTableID    string
	}{
		region:          "reg1",
		project:         "prj1",
		ncID:            "ncID1",
		netID:           "netID",
		peerAccID:       "peerAccID",
		peerVpcID:       "peerVpcID",
		peerIpv6:        "peerIpv6",
		peerIpv4:        "peerIpv4",
		peerRegion:      "reg2",
		netName:         "name",
		managedNetIpv4:  "ipv4_1",
		managedNetIpv6:  "ipv6_1",
		managedNetVpcID: "vpc2",
		peeringID:       "peeringID",
		routeTableID:    "routeTableID",
	}
)

func TestCreateNetworkConnection(t *testing.T) {
	l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	ctx := context.Background()
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	m := ec2mocks.NewMockEC2API(ctrl)
	mockEc2CreateNetworkConnection(t, m)
	provider := awsproviders.Provider{Ec2: m}
	providers := awsproviders.Providers{Controlplane: &provider, Dataplane: &provider}

	db := vpcdbmocks.NewMockVPCDB(ctrl)
	mockVPCDBCreateNetworkConnection(db, t)
	s := aws.NewCustomService(providers, l, db, config.AwsControlPlaneConfig{})
	op := models.Operation{
		Provider:  models.ProviderAWS,
		Region:    createNetworkConnectionConsts.region,
		ProjectID: createNetworkConnectionConsts.project,
		Params:    &models.CreateNetworkConnectionOperationParams{NetworkConnectionID: createNetworkConnectionConsts.ncID},
		State:     awsmodels.DefaultCreateNetworkConnectionOperationState(),
	}

	require.Error(t, s.CreateNetworkConnection(ctx, op))
	require.NoError(t, s.CreateNetworkConnection(ctx, op))
}

func mockVPCDBCreateNetworkConnection(db *vpcdbmocks.MockVPCDB, t *testing.T) {
	db.EXPECT().NetworkConnectionByID(gomock.Any(), createNetworkConnectionConsts.ncID).Return(
		models.NetworkConnection{
			ID:        createNetworkConnectionConsts.ncID,
			NetworkID: createNetworkConnectionConsts.netID,
			ProjectID: createNetworkConnectionConsts.project,
			Region:    createNetworkConnectionConsts.region,
			Provider:  models.ProviderAWS,
			Status:    models.NetworkConnectionStatusCreating,
			Params: &awsmodels.NetworkConnectionParams{
				Type:                awsmodels.NetworkConnectionPeering,
				AccountID:           createNetworkConnectionConsts.peerAccID,
				VpcID:               createNetworkConnectionConsts.peerVpcID,
				IPv4:                createNetworkConnectionConsts.peerIpv4,
				IPv6:                createNetworkConnectionConsts.peerIpv6,
				Region:              createNetworkConnectionConsts.peerRegion,
				PeeringConnectionID: createNetworkConnectionConsts.peeringID,
			},
		}, nil,
	)

	db.EXPECT().NetworkByID(gomock.Any(), createNetworkConnectionConsts.netID).Return(
		models.Network{
			ID:                createNetworkConnectionConsts.netID,
			ProjectID:         createNetworkConnectionConsts.project,
			Provider:          models.ProviderAWS,
			Name:              createNetworkConnectionConsts.netName,
			IPv4:              createNetworkConnectionConsts.managedNetIpv4,
			IPv6:              createNetworkConnectionConsts.managedNetIpv6,
			ExternalResources: &awsmodels.NetworkExternalResources{VpcID: createNetworkConnectionConsts.managedNetVpcID},
		}, nil,
	)

	db.EXPECT().MarkNetworkConnectionPending(gomock.Any(), createNetworkConnectionConsts.ncID, gomock.Any(), gomock.Any()).DoAndReturn(
		func(_ context.Context, _ string, _ string, params models.NetworkConnectionParams) error {
			p, ok := params.(*awsmodels.NetworkConnectionParams)
			require.True(t, ok)
			require.Equal(t, createNetworkConnectionConsts.peeringID, p.PeeringConnectionID)
			require.Equal(t, createNetworkConnectionConsts.managedNetIpv4, p.ManagedIPv4)
			require.Equal(t, createNetworkConnectionConsts.managedNetIpv6, p.ManagedIPv6)
			return nil
		})

	db.EXPECT().FinishNetworkConnectionCreating(gomock.Any(), createNetworkConnectionConsts.ncID).Return(nil)
}

func mockEc2CreateNetworkConnection(t *testing.T, api *ec2mocks.MockEC2API) {
	api.EXPECT().DescribeRouteTablesWithContext(gomock.Any(), gomock.Any()).DoAndReturn(
		func(_ context.Context, input *ec2.DescribeRouteTablesInput, _ ...request.Option) (*ec2.DescribeRouteTablesOutput, error) {
			require.Len(t, input.Filters, 1)
			require.Len(t, input.Filters[0].Values, 1)
			require.Equal(t, "vpc-id", *input.Filters[0].Name)
			require.Equal(t, createNetworkConnectionConsts.managedNetVpcID, *input.Filters[0].Values[0])
			return &ec2.DescribeRouteTablesOutput{RouteTables: []*ec2.RouteTable{{RouteTableId: &createNetworkConnectionConsts.routeTableID}}}, nil
		})

	api.EXPECT().CreateRouteWithContext(gomock.Any(), gomock.Any()).DoAndReturn(
		func(_ context.Context, input *ec2.CreateRouteInput, _ ...request.Option) (*ec2.CreateRouteOutput, error) {
			require.Equal(t, createNetworkConnectionConsts.routeTableID, *input.RouteTableId)
			require.Equal(t, createNetworkConnectionConsts.peerIpv4, *input.DestinationCidrBlock)
			require.Equal(t, createNetworkConnectionConsts.peeringID, *input.VpcPeeringConnectionId)
			return nil, nil
		})

	api.EXPECT().CreateRouteWithContext(gomock.Any(), gomock.Any()).DoAndReturn(
		func(_ context.Context, input *ec2.CreateRouteInput, _ ...request.Option) (*ec2.CreateRouteOutput, error) {
			require.Equal(t, createNetworkConnectionConsts.routeTableID, *input.RouteTableId)
			require.Equal(t, createNetworkConnectionConsts.peerIpv6, *input.DestinationIpv6CidrBlock)
			require.Equal(t, createNetworkConnectionConsts.peeringID, *input.VpcPeeringConnectionId)
			return nil, nil
		})

	api.EXPECT().DescribeVpcPeeringConnectionsWithContext(gomock.Any(), gomock.Any()).DoAndReturn(
		func(_ context.Context, intput *ec2.DescribeVpcPeeringConnectionsInput, _ ...request.Option) (*ec2.DescribeVpcPeeringConnectionsOutput, error) {
			require.Len(t, intput.VpcPeeringConnectionIds, 1)
			require.Equal(t, createNetworkConnectionConsts.peeringID, *intput.VpcPeeringConnectionIds[0])

			return &ec2.DescribeVpcPeeringConnectionsOutput{
				VpcPeeringConnections: []*ec2.VpcPeeringConnection{
					{
						Status: &ec2.VpcPeeringConnectionStateReason{
							Code: awslib.String(ec2.VpcPeeringConnectionStateReasonCodePendingAcceptance),
						},
					},
				},
			}, nil
		})

	api.EXPECT().DescribeVpcPeeringConnectionsWithContext(gomock.Any(), gomock.Any()).DoAndReturn(
		func(_ context.Context, intput *ec2.DescribeVpcPeeringConnectionsInput, _ ...request.Option) (*ec2.DescribeVpcPeeringConnectionsOutput, error) {
			require.Len(t, intput.VpcPeeringConnectionIds, 1)
			require.Equal(t, createNetworkConnectionConsts.peeringID, *intput.VpcPeeringConnectionIds[0])

			return &ec2.DescribeVpcPeeringConnectionsOutput{
				VpcPeeringConnections: []*ec2.VpcPeeringConnection{
					{
						Status: &ec2.VpcPeeringConnectionStateReason{
							Code: awslib.String(ec2.VpcPeeringConnectionStateReasonCodeActive),
						},
					},
				},
			}, nil
		})
}
