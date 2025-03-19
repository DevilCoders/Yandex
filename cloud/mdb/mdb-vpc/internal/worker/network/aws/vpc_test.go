package aws_test

import (
	"context"
	"testing"

	aws2 "github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/aws/aws-sdk-go/service/ec2/ec2iface"
	"github.com/stretchr/testify/require"

	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws3 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type mockEC2Client struct {
	ec2iface.EC2API
	t *testing.T

	createVpcCount int
	createVpcMocks []func(context.Context, *ec2.CreateVpcInput, ...request.Option) (*ec2.CreateVpcOutput, error)

	getVpcCount int
	getVpcMocks []func(context.Context, *ec2.DescribeVpcsInput, ...request.Option) (*ec2.DescribeVpcsOutput, error)

	createSubnetCount int
	createSubnetMocks []func(context.Context, *ec2.CreateSubnetInput, ...request.Option) (*ec2.CreateSubnetOutput, error)

	getSubnetCount int
	getSubnetMocks []func(context.Context, *ec2.DescribeSubnetsInput, ...request.Option) (*ec2.DescribeSubnetsOutput, error)

	modifySubnetCount int
	modifySubnetMocks []func(context.Context, *ec2.ModifySubnetAttributeInput, ...request.Option) (*ec2.ModifySubnetAttributeOutput, error)

	createIgwCount int
	createIgwMocks []func(context.Context, *ec2.CreateInternetGatewayInput, ...request.Option) (*ec2.CreateInternetGatewayOutput, error)

	attachIgwCount int
	attachIgwMocks []func(*testing.T, context.Context, *ec2.AttachInternetGatewayInput, ...request.Option) (*ec2.AttachInternetGatewayOutput, error)
}

func (m *mockEC2Client) CreateVpcWithContext(ctx context.Context, input *ec2.CreateVpcInput, opts ...request.Option) (*ec2.CreateVpcOutput, error) {
	res, err := m.createVpcMocks[m.createVpcCount](ctx, input, opts...)
	m.createVpcCount++
	return res, err
}

func (m *mockEC2Client) DescribeVpcsWithContext(ctx context.Context, input *ec2.DescribeVpcsInput, opts ...request.Option) (*ec2.DescribeVpcsOutput, error) {
	res, err := m.getVpcMocks[m.getVpcCount](ctx, input, opts...)
	m.getVpcCount++
	return res, err
}

func TestService_CreateVpc(t *testing.T) {
	l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	type input struct {
		createVpcMocks []func(context.Context, *ec2.CreateVpcInput, ...request.Option) (*ec2.CreateVpcOutput, error)
		getVpcMocks    []func(context.Context, *ec2.DescribeVpcsInput, ...request.Option) (*ec2.DescribeVpcsOutput, error)
		op             models.Operation
		state          *aws3.CreateNetworkOperationState
	}
	type expected struct {
		net *aws3.Vpc
		err error
	}
	tcs := []struct {
		name     string
		input    input
		expected expected
	}{
		{
			name: "simple create",
			input: input{
				createVpcMocks: []func(context.Context, *ec2.CreateVpcInput, ...request.Option) (*ec2.CreateVpcOutput, error){
					func(ctx context.Context, input *ec2.CreateVpcInput, option ...request.Option) (*ec2.CreateVpcOutput, error) {
						return &ec2.CreateVpcOutput{Vpc: &ec2.Vpc{
							CidrBlock: input.CidrBlock,
							VpcId:     aws2.String("qwerty"),
							State:     aws2.String("available"),
							Ipv6CidrBlockAssociationSet: []*ec2.VpcIpv6CidrBlockAssociation{
								{
									Ipv6CidrBlock: aws2.String("::ffff/128"),
								},
							},
						}}, nil
					},
				},
				op: models.Operation{
					ProjectID: "p1",
				},
				state: &aws3.CreateNetworkOperationState{Network: aws3.NetworkState{IPv4: "1.1.1.1/10"}},
			},
			expected: expected{
				net: &aws3.Vpc{
					ID:       "qwerty",
					IPv4CIDR: "1.1.1.1/10",
					IPv6CIDR: "::ffff/128",
				},
			},
		},
		{
			name: "wait create",
			input: input{
				createVpcMocks: []func(context.Context, *ec2.CreateVpcInput, ...request.Option) (*ec2.CreateVpcOutput, error){
					func(ctx context.Context, input *ec2.CreateVpcInput, option ...request.Option) (*ec2.CreateVpcOutput, error) {
						return &ec2.CreateVpcOutput{Vpc: &ec2.Vpc{
							CidrBlock: input.CidrBlock,
							VpcId:     aws2.String("qwerty"),
							State:     aws2.String("creating"),
						}}, nil
					},
				},
				getVpcMocks: []func(context.Context, *ec2.DescribeVpcsInput, ...request.Option) (*ec2.DescribeVpcsOutput, error){
					func(ctx context.Context, input *ec2.DescribeVpcsInput, option ...request.Option) (*ec2.DescribeVpcsOutput, error) {
						return &ec2.DescribeVpcsOutput{
							Vpcs: []*ec2.Vpc{
								{
									CidrBlock: aws2.String("1.1.1.1/10"),
									VpcId:     input.VpcIds[0],
									State:     aws2.String("creating"),
								},
							},
						}, nil
					},
					func(ctx context.Context, input *ec2.DescribeVpcsInput, option ...request.Option) (*ec2.DescribeVpcsOutput, error) {
						return &ec2.DescribeVpcsOutput{
							Vpcs: []*ec2.Vpc{
								{
									CidrBlock: aws2.String("1.1.1.1/10"),
									VpcId:     input.VpcIds[0],
									State:     aws2.String("creating"),
								},
							},
						}, nil
					},
					func(ctx context.Context, input *ec2.DescribeVpcsInput, option ...request.Option) (*ec2.DescribeVpcsOutput, error) {
						return &ec2.DescribeVpcsOutput{
							Vpcs: []*ec2.Vpc{
								{
									CidrBlock: aws2.String("1.1.1.1/10"),
									VpcId:     input.VpcIds[0],
									State:     aws2.String("available"),
									Ipv6CidrBlockAssociationSet: []*ec2.VpcIpv6CidrBlockAssociation{
										{
											Ipv6CidrBlock: aws2.String("::ffff/128"),
										},
									},
								},
							},
						}, nil
					},
				},
				op: models.Operation{
					ProjectID: "p1",
				},
				state: &aws3.CreateNetworkOperationState{Network: aws3.NetworkState{IPv4: "1.1.1.1/10"}},
			},
			expected: expected{
				net: &aws3.Vpc{
					ID:       "qwerty",
					IPv4CIDR: "1.1.1.1/10",
					IPv6CIDR: "::ffff/128",
				},
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()

			m := &mockEC2Client{
				createVpcMocks: tc.input.createVpcMocks,
				getVpcMocks:    tc.input.getVpcMocks,
			}
			provider := awsproviders.Provider{Ec2: m}
			providers := awsproviders.Providers{Controlplane: &provider, Dataplane: &provider}
			s := aws.NewCustomService(providers, l, nil, config.AwsControlPlaneConfig{})
			err := s.CreateVpc(ctx, tc.input.op, tc.input.state)
			if tc.expected.err == nil {
				require.NoError(t, err)
				require.Equal(t, tc.expected.net, tc.input.state.Vpc)
			} else {
				require.ErrorAs(t, tc.expected.err, err)
			}
		})
	}
}
