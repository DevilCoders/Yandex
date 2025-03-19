package aws

import (
	"context"
	"fmt"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/aws/aws-sdk-go/service/iam"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ImportIamActions = aws.StringSlice([]string{
		"ec2:*",
	})
)

func (v *Validator) ValidateImportVPCData(ctx context.Context, params interface{}) (validation.ValidateImportVPCResult, error) {
	awsParams := params.(*network.ImportNetworkRequest_Aws).Aws

	ec2Cli := v.ec2CliFactory(awsParams.IamRoleArn, awsParams.RegionId, v.client)
	iamCli := v.iamCliFactory(awsParams.IamRoleArn, awsParams.RegionId, v.client)

	vpcData, err := ec2Cli.DescribeVpcsWithContext(ctx, &ec2.DescribeVpcsInput{
		VpcIds: aws.StringSlice([]string{awsParams.VpcId}),
	})
	if err != nil {
		return validation.ValidateImportVPCResult{}, xerrors.Errorf("get vpc: %w", err)
	}

	if len(vpcData.Vpcs) == 0 {
		return validation.ValidateImportVPCResult{}, semerr.InvalidInputf("vpc %q does not exist", awsParams.VpcId)
	}

	if len(vpcData.Vpcs[0].Ipv6CidrBlockAssociationSet) == 0 {
		return validation.ValidateImportVPCResult{}, semerr.InvalidInputf("vpc %q does not have IPv6 CIDR block", awsParams.VpcId)
	}

	iamData, err := iamCli.SimulatePrincipalPolicyWithContext(ctx, &iam.SimulatePrincipalPolicyInput{
		ActionNames:     ImportIamActions,
		PolicySourceArn: aws.String(awsParams.IamRoleArn),
		ResourceArns: aws.StringSlice([]string{fmt.Sprintf("arn:aws:ec2:%s:%s:vpc/%s",
			awsParams.RegionId,
			awsParams.AccountId,
			awsParams.VpcId,
		)}),
	})
	if err != nil {
		return validation.ValidateImportVPCResult{}, semerr.WrapWithInvalidInput(err, "simulate principal policy")
	}

	for _, res := range iamData.EvaluationResults {
		if aws.StringValue(res.EvalDecision) != iam.PolicyEvaluationDecisionTypeAllowed {
			return validation.ValidateImportVPCResult{}, semerr.InvalidInputf("%q can not do %q on %q",
				awsParams.IamRoleArn,
				aws.StringValue(res.EvalActionName),
				aws.StringValue(res.EvalResourceName),
			)
		}
	}

	return validation.ValidateImportVPCResult{
		IPv4CIDRBlock: aws.StringValue(vpcData.Vpcs[0].CidrBlock),
	}, nil
}
