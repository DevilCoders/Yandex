package aws_test

import (
	"context"
	"testing"

	awssdk "github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/stretchr/testify/require"

	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func (m *mockEC2Client) CreateInternetGatewayWithContext(ctx context.Context, input *ec2.CreateInternetGatewayInput, opts ...request.Option) (*ec2.CreateInternetGatewayOutput, error) {
	res, err := m.createIgwMocks[m.createIgwCount](ctx, input, opts...)
	m.createIgwCount++
	return res, err
}

func (m *mockEC2Client) AttachInternetGatewayWithContext(ctx context.Context, input *ec2.AttachInternetGatewayInput, opts ...request.Option) (*ec2.AttachInternetGatewayOutput, error) {
	res, err := m.attachIgwMocks[m.attachIgwCount](m.t, ctx, input, opts...)
	m.attachIgwCount++
	return res, err
}

func TestService_CreateInternetGateway(t *testing.T) {
	l, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))

	type input struct {
		createIgwMocks []func(ctx context.Context, input *ec2.CreateInternetGatewayInput, opts ...request.Option) (*ec2.CreateInternetGatewayOutput, error)
		attachIgwMocks []func(t *testing.T, ctx context.Context, input *ec2.AttachInternetGatewayInput, opts ...request.Option) (*ec2.AttachInternetGatewayOutput, error)
		op             models.Operation
		state          *awsmodels.CreateNetworkOperationState
	}
	type output struct {
		err  error
		gwID string
	}

	tcs := []struct {
		name   string
		input  input
		output output
	}{
		{
			name: "simple create",
			input: input{
				createIgwMocks: []func(ctx context.Context, input *ec2.CreateInternetGatewayInput, opts ...request.Option) (*ec2.CreateInternetGatewayOutput, error){
					func(ctx context.Context, input *ec2.CreateInternetGatewayInput, opts ...request.Option) (*ec2.CreateInternetGatewayOutput, error) {
						return &ec2.CreateInternetGatewayOutput{InternetGateway: &ec2.InternetGateway{InternetGatewayId: awssdk.String("id1")}}, nil
					},
				},
				attachIgwMocks: []func(t *testing.T, ctx context.Context, input *ec2.AttachInternetGatewayInput, opts ...request.Option) (*ec2.AttachInternetGatewayOutput, error){
					func(t *testing.T, ctx context.Context, input *ec2.AttachInternetGatewayInput, opts ...request.Option) (*ec2.AttachInternetGatewayOutput, error) {
						require.Equal(t, "vpc1", awssdk.StringValue(input.VpcId))
						return &ec2.AttachInternetGatewayOutput{}, nil
					},
				},
				op: models.Operation{
					ProjectID: "p1",
				},
				state: &awsmodels.CreateNetworkOperationState{
					Vpc: &awsmodels.Vpc{ID: "vpc1"},
				},
			},
			output: output{
				gwID: "id1",
			},
		},
	}

	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()

			provider := &mockEC2Client{
				t:              t,
				createIgwMocks: tc.input.createIgwMocks,
				attachIgwMocks: tc.input.attachIgwMocks,
			}
			prov := awsproviders.Provider{Ec2: provider}
			providers := awsproviders.Providers{Controlplane: &prov, Dataplane: &prov}
			s := aws.NewCustomService(providers, l, nil, config.AwsControlPlaneConfig{})
			err := s.CreateInternetGateway(ctx, tc.input.op, tc.input.state)
			if tc.output.err != nil {
				require.ErrorAs(t, err, tc.output.err)
			} else {
				require.NoError(t, err)
				require.Equal(t, tc.output.gwID, tc.input.state.IgwID)
			}
		})
	}
}
