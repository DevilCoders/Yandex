package aws_test

import (
	"context"
	"fmt"
	"testing"

	aws2 "github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/stretchr/testify/require"

	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	aws3 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func (m *mockEC2Client) CreateSubnetWithContext(ctx context.Context, input *ec2.CreateSubnetInput, opts ...request.Option) (*ec2.CreateSubnetOutput, error) {
	res, err := m.createSubnetMocks[m.createSubnetCount](ctx, input, opts...)
	m.createSubnetCount++
	return res, err
}

func (m *mockEC2Client) DescribeSubnetsWithContext(ctx context.Context, input *ec2.DescribeSubnetsInput, opts ...request.Option) (*ec2.DescribeSubnetsOutput, error) {
	res, err := m.getSubnetMocks[m.getSubnetCount](ctx, input, opts...)
	m.getSubnetCount++
	return res, err
}

func (m *mockEC2Client) ModifySubnetAttributeWithContext(ctx context.Context, input *ec2.ModifySubnetAttributeInput, opts ...request.Option) (*ec2.ModifySubnetAttributeOutput, error) {
	res, err := m.modifySubnetMocks[m.modifySubnetCount](ctx, input, opts...)
	m.modifySubnetCount++
	return res, err
}

func simpleCreateSubnetsMock(n int) (res []func(context.Context, *ec2.CreateSubnetInput, ...request.Option) (*ec2.CreateSubnetOutput, error)) {
	for i := 1; i <= n; i++ {
		x := i
		res = append(res, func(ctx context.Context, input *ec2.CreateSubnetInput, option ...request.Option) (*ec2.CreateSubnetOutput, error) {
			return &ec2.CreateSubnetOutput{Subnet: &ec2.Subnet{
				AvailabilityZoneId: input.AvailabilityZoneId,
				CidrBlock:          input.CidrBlock,
				Ipv6CidrBlockAssociationSet: []*ec2.SubnetIpv6CidrBlockAssociation{{
					Ipv6CidrBlock: input.Ipv6CidrBlock,
				}},
				State:    aws2.String("available"),
				SubnetId: aws2.String(fmt.Sprintf("id%d", x)),
			}}, nil
		})
	}
	return res
}

func simpleModifySubnetsMock(n int) (res []func(context.Context, *ec2.ModifySubnetAttributeInput, ...request.Option) (*ec2.ModifySubnetAttributeOutput, error)) {
	for i := 0; i < n; i++ {
		res = append(res,
			func(ctx context.Context, input *ec2.ModifySubnetAttributeInput, option ...request.Option) (*ec2.ModifySubnetAttributeOutput, error) {
				return nil, nil
			},
			func(ctx context.Context, input *ec2.ModifySubnetAttributeInput, option ...request.Option) (*ec2.ModifySubnetAttributeOutput, error) {
				return nil, nil
			},
		)
	}
	return res
}

func TestCreateSubnets(t *testing.T) {
	l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	storeCreateInputs := make([]*ec2.CreateSubnetInput, 0)
	type input struct {
		createSubnetMocks []func(context.Context, *ec2.CreateSubnetInput, ...request.Option) (*ec2.CreateSubnetOutput, error)
		getSubnetMocks    []func(context.Context, *ec2.DescribeSubnetsInput, ...request.Option) (*ec2.DescribeSubnetsOutput, error)
		modifySubnetMocks []func(context.Context, *ec2.ModifySubnetAttributeInput, ...request.Option) (*ec2.ModifySubnetAttributeOutput, error)
		op                models.Operation
		state             *aws3.CreateNetworkOperationState
	}
	type expected struct {
		nets []*aws3.Subnet
		err  error
	}
	tcs := []struct {
		name     string
		input    input
		expected expected
	}{
		{
			name: "simple create",
			input: input{
				createSubnetMocks: simpleCreateSubnetsMock(1),
				modifySubnetMocks: simpleModifySubnetsMock(1),
				op: models.Operation{
					ProjectID: "p1",
				},
				state: &aws3.CreateNetworkOperationState{
					Vpc: &aws3.Vpc{
						ID:       "vpc1",
						IPv4CIDR: "10.0.0.0/16",
						IPv6CIDR: "fc00::/56",
					},
					Zones: []string{"a"},
				},
			},
			expected: expected{
				nets: []*aws3.Subnet{
					{
						ID:       "id1",
						IPv4CIDR: "10.0.0.0/18",
						IPv6CIDR: "fc00::/64",
						AZ:       "a",
					},
				},
			},
		},
		{
			name: "wait create",
			input: input{
				createSubnetMocks: []func(context.Context, *ec2.CreateSubnetInput, ...request.Option) (*ec2.CreateSubnetOutput, error){
					func(ctx context.Context, input *ec2.CreateSubnetInput, option ...request.Option) (*ec2.CreateSubnetOutput, error) {
						storeCreateInputs = append(storeCreateInputs, input)
						return &ec2.CreateSubnetOutput{Subnet: &ec2.Subnet{
							SubnetId: aws2.String("id1"),
							State:    aws2.String("creating"),
						}}, nil
					},
					func(ctx context.Context, input *ec2.CreateSubnetInput, option ...request.Option) (*ec2.CreateSubnetOutput, error) {
						storeCreateInputs = append(storeCreateInputs, input)
						return &ec2.CreateSubnetOutput{Subnet: &ec2.Subnet{
							SubnetId: aws2.String("id2"),
							State:    aws2.String("creating"),
						}}, nil
					},
				},
				modifySubnetMocks: []func(context.Context, *ec2.ModifySubnetAttributeInput, ...request.Option) (*ec2.ModifySubnetAttributeOutput, error){
					func(ctx context.Context, input *ec2.ModifySubnetAttributeInput, opts ...request.Option) (*ec2.ModifySubnetAttributeOutput, error) {
						return nil, nil
					},
					func(ctx context.Context, input *ec2.ModifySubnetAttributeInput, opts ...request.Option) (*ec2.ModifySubnetAttributeOutput, error) {
						return nil, nil
					},
					func(ctx context.Context, input *ec2.ModifySubnetAttributeInput, opts ...request.Option) (*ec2.ModifySubnetAttributeOutput, error) {
						return nil, nil
					},
					func(ctx context.Context, input *ec2.ModifySubnetAttributeInput, opts ...request.Option) (*ec2.ModifySubnetAttributeOutput, error) {
						return nil, nil
					},
				},
				getSubnetMocks: []func(context.Context, *ec2.DescribeSubnetsInput, ...request.Option) (*ec2.DescribeSubnetsOutput, error){
					func(ctx context.Context, input *ec2.DescribeSubnetsInput, option ...request.Option) (*ec2.DescribeSubnetsOutput, error) {
						return &ec2.DescribeSubnetsOutput{Subnets: []*ec2.Subnet{{
							State: aws2.String("creating"),
						}}}, nil
					},
					func(ctx context.Context, input *ec2.DescribeSubnetsInput, option ...request.Option) (*ec2.DescribeSubnetsOutput, error) {
						return &ec2.DescribeSubnetsOutput{Subnets: []*ec2.Subnet{{
							AvailabilityZoneId: storeCreateInputs[0].AvailabilityZoneId,
							CidrBlock:          storeCreateInputs[0].CidrBlock,
							Ipv6CidrBlockAssociationSet: []*ec2.SubnetIpv6CidrBlockAssociation{{
								Ipv6CidrBlock: storeCreateInputs[0].Ipv6CidrBlock,
							}},
							State:    aws2.String("available"),
							SubnetId: input.SubnetIds[0],
						}}}, nil
					},
					func(ctx context.Context, input *ec2.DescribeSubnetsInput, option ...request.Option) (*ec2.DescribeSubnetsOutput, error) {
						return &ec2.DescribeSubnetsOutput{Subnets: []*ec2.Subnet{{
							AvailabilityZoneId: storeCreateInputs[1].AvailabilityZoneId,
							CidrBlock:          storeCreateInputs[1].CidrBlock,
							Ipv6CidrBlockAssociationSet: []*ec2.SubnetIpv6CidrBlockAssociation{{
								Ipv6CidrBlock: storeCreateInputs[1].Ipv6CidrBlock,
							}},
							State:    aws2.String("available"),
							SubnetId: input.SubnetIds[0],
						}}}, nil
					},
				},
				op: models.Operation{
					ProjectID: "p1",
				},
				state: &aws3.CreateNetworkOperationState{
					Vpc: &aws3.Vpc{
						ID:       "vpc1",
						IPv4CIDR: "10.0.0.0/16",
						IPv6CIDR: "fc00::/56",
					},
					Zones: []string{"a", "b"},
				},
			},
			expected: expected{
				nets: []*aws3.Subnet{
					{
						ID:       "id1",
						IPv4CIDR: "10.0.0.0/18",
						IPv6CIDR: "fc00::/64",
						AZ:       "a",
					},
					{
						ID:       "id2",
						IPv4CIDR: "10.0.64.0/18",
						IPv6CIDR: "fc00:0:0:1::/64",
						AZ:       "b",
					},
				},
			},
		},
		{
			name: "create network in 3 az",
			input: input{
				createSubnetMocks: simpleCreateSubnetsMock(3),
				modifySubnetMocks: simpleModifySubnetsMock(3),
				op: models.Operation{
					ProjectID: "p1",
				},
				state: &aws3.CreateNetworkOperationState{
					Vpc: &aws3.Vpc{
						ID:       "vpc1",
						IPv4CIDR: "10.0.0.0/16",
						IPv6CIDR: "fc00::/56",
					},
					Zones: []string{"a", "b", "c"},
				},
			},
			expected: expected{
				nets: []*aws3.Subnet{
					{
						ID:       "id1",
						IPv4CIDR: "10.0.0.0/18",
						IPv6CIDR: "fc00::/64",
						AZ:       "a",
					},
					{
						ID:       "id2",
						IPv4CIDR: "10.0.64.0/18",
						IPv6CIDR: "fc00:0:0:1::/64",
						AZ:       "b",
					},
					{
						ID:       "id3",
						IPv4CIDR: "10.0.128.0/18",
						IPv6CIDR: "fc00:0:0:2::/64",
						AZ:       "c",
					},
				},
			},
		},
		{
			name: "create network in 6 az",
			input: input{
				createSubnetMocks: simpleCreateSubnetsMock(6),
				modifySubnetMocks: simpleModifySubnetsMock(6),
				op: models.Operation{
					ProjectID: "p1",
				},
				state: &aws3.CreateNetworkOperationState{
					Vpc: &aws3.Vpc{
						ID:       "vpc1",
						IPv4CIDR: "10.0.0.0/16",
						IPv6CIDR: "fc00::/56",
					},
					Zones: []string{"a", "b", "c", "d", "e", "f"},
				},
			},
			expected: expected{
				nets: []*aws3.Subnet{
					{
						ID:       "id1",
						IPv4CIDR: "10.0.0.0/19",
						IPv6CIDR: "fc00::/64",
						AZ:       "a",
					},
					{
						ID:       "id2",
						IPv4CIDR: "10.0.32.0/19",
						IPv6CIDR: "fc00:0:0:1::/64",
						AZ:       "b",
					},
					{
						ID:       "id3",
						IPv4CIDR: "10.0.64.0/19",
						IPv6CIDR: "fc00:0:0:2::/64",
						AZ:       "c",
					},
					{
						ID:       "id4",
						IPv4CIDR: "10.0.96.0/19",
						IPv6CIDR: "fc00:0:0:3::/64",
						AZ:       "d",
					},
					{
						ID:       "id5",
						IPv4CIDR: "10.0.128.0/19",
						IPv6CIDR: "fc00:0:0:4::/64",
						AZ:       "e",
					},
					{
						ID:       "id6",
						IPv4CIDR: "10.0.160.0/19",
						IPv6CIDR: "fc00:0:0:5::/64",
						AZ:       "f",
					},
				},
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()

			provider := &mockEC2Client{
				createSubnetMocks: tc.input.createSubnetMocks,
				getSubnetMocks:    tc.input.getSubnetMocks,
				modifySubnetMocks: tc.input.modifySubnetMocks,
			}
			prov := awsproviders.Provider{Ec2: provider}
			providers := awsproviders.Providers{Controlplane: &prov, Dataplane: &prov}
			s := aws.NewCustomService(providers, l, nil, config.AwsControlPlaneConfig{})
			storeCreateInputs = make([]*ec2.CreateSubnetInput, 0)
			err := s.CreateSubnets(ctx, tc.input.op, tc.input.state)
			if tc.expected.err == nil {
				require.NoError(t, err)
				require.Equal(t, tc.expected.nets, tc.input.state.Subnets)
			} else {
				require.ErrorAs(t, tc.expected.err, err)
			}
		})
	}
}
