package aws_test

import (
	"context"
	"net/http"
	"testing"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/aws/aws-sdk-go/service/ec2/ec2iface"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	ec2mocks "a.yandex-team.ru/cloud/mdb/internal/x/github.com/aws/aws-sdk-go/service/ec2/ec2iface/mocks"
	awsvalidator "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
)

func TestValidateCreateNetworkConnectionData(t *testing.T) {
	ctx := context.Background()
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	vpcID := "vpc-id"
	regionID := "region"
	peerRegion := "peerReg"
	peerVpc := "peerVpc"
	peerOwner := "peerOwner"
	peerIPv4 := "127.0.0.1/32"
	peerIPv6 := "::/128"
	peeringID := "peeringID"

	ec2Cli := ec2mocks.NewMockEC2API(ctrl)

	ec2Cli.EXPECT().CreateVpcPeeringConnectionWithContext(gomock.Any(), &ec2.CreateVpcPeeringConnectionInput{
		PeerRegion:  aws.String(peerRegion),
		PeerVpcId:   aws.String(peerVpc),
		PeerOwnerId: aws.String(peerOwner),
		VpcId:       aws.String(vpcID),
	}).Return(
		&ec2.CreateVpcPeeringConnectionOutput{
			VpcPeeringConnection: &ec2.VpcPeeringConnection{
				VpcPeeringConnectionId: aws.String(peeringID),
			},
		}, nil)

	validator := awsvalidator.NewValidator(nil, nil, "",
		awsvalidator.WithEc2CliFactory(func(_ string, _ string, _ *http.Client) ec2iface.EC2API {
			return ec2Cli
		}),
	)

	res, err := validator.ValidateCreateNetworkConnectionData(
		ctx,
		models.Network{
			Region: regionID,
			ExternalResources: &awsmodels.NetworkExternalResources{
				VpcID: vpcID,
			},
		},
		&network.CreateNetworkConnectionRequest_Aws{
			Aws: &network.CreateAWSNetworkConnectionRequest{
				Type: &network.CreateAWSNetworkConnectionRequest_Peering{
					Peering: &network.CreateAWSNetworkConnectionPeeringRequest{
						VpcId:         peerVpc,
						AccountId:     peerOwner,
						RegionId:      peerRegion,
						Ipv4CidrBlock: peerIPv4,
						Ipv6CidrBlock: peerIPv6,
					},
				},
			},
		})

	require.NoError(t, err)
	require.Equal(t, peeringID, res.PeeringID)
}
