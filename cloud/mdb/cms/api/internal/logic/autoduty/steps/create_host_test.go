package steps

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/conductor/mocks"
)

func TestHostInConductorStep_RunStep(t *testing.T) {
	type fields struct {
		conductor conductor.Client
		groupID   int
	}
	type args struct {
		ctx     context.Context
		execCtx *InstructionCtx
	}
	testFqdn := "vla1-test-fqdn"
	ctx := context.Background()
	tests := []struct {
		name   string
		fields func(ctrl *gomock.Controller) fields
		args   args
		want   RunResult
	}{
		{
			name: "already exists",
			fields: func(ctrl *gomock.Controller) fields {
				cond := mocks.NewMockClient(ctrl)
				cond.EXPECT().HostsInfo(gomock.Any(), []string{testFqdn}).Return([]conductor.HostInfo{{}}, nil)
				return fields{
					conductor: cond,
					groupID:   0,
				}
			},
			args: args{
				ctx: ctx,
				execCtx: &InstructionCtx{
					rd: &types.RequestDecisionTuple{
						R: models.ManagementRequest{
							Fqnds: []string{testFqdn},
						},
						D: models.AutomaticDecision{},
					},
				},
			},
			want: RunResult{
				ForHuman: "already exists",
				Action:   AfterStepContinue,
			},
		},
		{
			name: "1 host added",
			fields: func(ctrl *gomock.Controller) fields {
				cond := mocks.NewMockClient(ctrl)
				cond.EXPECT().DCByName(gomock.Any(), "vla").Return(conductor.DataCenter{ID: 1}, nil)
				cond.EXPECT().HostsInfo(gomock.Any(), []string{testFqdn}).Return(nil, nil)
				cond.EXPECT().HostCreate(gomock.Any(), gomock.Any()).Return(nil)
				return fields{
					conductor: cond,
					groupID:   0,
				}
			},
			args: args{
				ctx: ctx,
				execCtx: &InstructionCtx{
					rd: &types.RequestDecisionTuple{
						R: models.ManagementRequest{
							Fqnds: []string{testFqdn},
						},
						D: models.AutomaticDecision{},
					},
				},
			},
			want: RunResult{
				ForHuman: "successfully created in conductor",
				Action:   AfterStepContinue,
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			f := tt.fields(ctrl)
			s := NewHostInConductorStep(f.conductor, NewHostsConfig{MDBDom0GroupID: f.groupID})
			r := s.RunStep(tt.args.ctx, tt.args.execCtx)
			require.Equal(t, tt.want, r)
		})
	}
}
