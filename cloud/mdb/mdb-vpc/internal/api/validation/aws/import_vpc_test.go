package aws_test

import (
	"context"
	"net/http"
	"testing"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/aws/aws-sdk-go/service/ec2/ec2iface"
	"github.com/aws/aws-sdk-go/service/iam"
	"github.com/aws/aws-sdk-go/service/iam/iamiface"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	ec2mocks "a.yandex-team.ru/cloud/mdb/internal/x/github.com/aws/aws-sdk-go/service/ec2/ec2iface/mocks"
	iammocks "a.yandex-team.ru/cloud/mdb/internal/x/github.com/aws/aws-sdk-go/service/iam/iamiface/mocks"
	awsvalidator "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation/aws"
)

func TestValidateImportVPCData(t *testing.T) {
	ctx := context.Background()
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	vpcID := "vpc-id"
	roleArn := "some-role"
	regionID := "region"
	accountID := "account-id"

	ec2Cli := ec2mocks.NewMockEC2API(ctrl)
	iamCli := iammocks.NewMockIAMAPI(ctrl)

	ec2Cli.EXPECT().DescribeVpcsWithContext(gomock.Any(), &ec2.DescribeVpcsInput{
		VpcIds: aws.StringSlice([]string{vpcID}),
	}).Return(&ec2.DescribeVpcsOutput{
		Vpcs: []*ec2.Vpc{
			{
				CidrBlock: aws.String("1.2.3.4/0"),
				Ipv6CidrBlockAssociationSet: []*ec2.VpcIpv6CidrBlockAssociation{
					{
						Ipv6CidrBlock: aws.String("::/0"),
					},
				},
			},
		},
	}, nil)

	iamCli.EXPECT().SimulatePrincipalPolicyWithContext(gomock.Any(), &iam.SimulatePrincipalPolicyInput{
		ActionNames:     awsvalidator.ImportIamActions,
		PolicySourceArn: aws.String(roleArn),
		ResourceArns:    aws.StringSlice([]string{"arn:aws:ec2:region:account-id:vpc/vpc-id"}),
	}).Return(&iam.SimulatePolicyResponse{
		EvaluationResults: []*iam.EvaluationResult{
			{
				EvalDecision: aws.String(iam.PolicyEvaluationDecisionTypeAllowed),
			},
		},
	}, nil)

	validator := awsvalidator.NewValidator(nil, nil, "",
		awsvalidator.WithEc2CliFactory(func(_ string, _ string, _ *http.Client) ec2iface.EC2API {
			return ec2Cli
		}),
		awsvalidator.WithIamCliFactory(func(_ string, _ string, _ *http.Client) iamiface.IAMAPI {
			return iamCli
		}),
	)
	res, err := validator.ValidateImportVPCData(ctx, &network.ImportNetworkRequest_Aws{
		Aws: &network.ImportAWSVPCRequest{
			VpcId:      vpcID,
			RegionId:   regionID,
			AccountId:  accountID,
			IamRoleArn: roleArn,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, res.IPv4CIDRBlock)
}
