package grpc

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"google.golang.org/protobuf/types/known/timestamppb"

	cmsv1 "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient"
	mockcms "a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb"
	mockmeta "a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
)

func TestGRPCCMS_AwaitingPrimarySwitchover(t *testing.T) {
	ctx := context.Background()
	type fields struct {
		cms  instanceclient.InstanceClient
		meta metadb.MetaDB
	}
	type args struct {
		duration    time.Duration
		clusterType string
	}
	tests := []struct {
		name   string
		fields func(ctrl *gomock.Controller) fields
		args   args
		want   []models.Cluster
	}{
		{
			name: "no task in cms",
			fields: func(ctrl *gomock.Controller) fields {
				meta := mockmeta.NewMockMetaDB(ctrl)
				cms := mockcms.NewMockInstanceClient(ctrl)
				cms.EXPECT().List(gomock.Any()).Return(
					&cmsv1.ListInstanceOperationsResponse{
						Operations: []*cmsv1.InstanceOperation{},
					}, nil)
				return fields{
					cms:  cms,
					meta: meta,
				}
			},
			args: args{},
			want: []models.Cluster(nil),
		},
		{
			name: "1 task whip primary not last",
			fields: func(ctrl *gomock.Controller) fields {
				meta := mockmeta.NewMockMetaDB(ctrl)
				cms := mockcms.NewMockInstanceClient(ctrl)
				cms.EXPECT().List(gomock.Any()).Return(
					&cmsv1.ListInstanceOperationsResponse{
						Operations: []*cmsv1.InstanceOperation{
							{
								ExecutedSteps: []cmsv1.InstanceOperation_StepName{
									cmsv1.InstanceOperation_CHECK_IF_PRIMARY,
									cmsv1.InstanceOperation_STEP_UNKNOWN,
								},
							},
						},
					}, nil)
				return fields{
					cms:  cms,
					meta: meta,
				}
			},
			args: args{},
			want: []models.Cluster(nil),
		},
		{
			name: "waits whip primary but not long enough",
			fields: func(ctrl *gomock.Controller) fields {
				meta := mockmeta.NewMockMetaDB(ctrl)
				cms := mockcms.NewMockInstanceClient(ctrl)
				cms.EXPECT().List(gomock.Any()).Return(
					&cmsv1.ListInstanceOperationsResponse{
						Operations: []*cmsv1.InstanceOperation{
							{
								CreatedAt: timestamppb.New(time.Now().Add(-time.Minute)),
								ExecutedSteps: []cmsv1.InstanceOperation_StepName{
									cmsv1.InstanceOperation_CHECK_IF_PRIMARY,
								},
							},
						},
					}, nil)
				return fields{
					cms:  cms,
					meta: meta,
				}
			},
			args: args{
				duration: 30 * time.Minute,
			},
			want: []models.Cluster(nil),
		},
		{
			name: "whip primary happy path",
			fields: func(ctrl *gomock.Controller) fields {
				testInstanceID := "test-instance"
				meta := mockmeta.NewMockMetaDB(ctrl)
				cms := mockcms.NewMockInstanceClient(ctrl)
				cms.EXPECT().List(gomock.Any()).Return(
					&cmsv1.ListInstanceOperationsResponse{
						Operations: []*cmsv1.InstanceOperation{
							{
								InstanceId: testInstanceID,
								CreatedAt:  timestamppb.New(time.Now().Add(-10 * time.Minute)),
								ExecutedSteps: []cmsv1.InstanceOperation_StepName{
									cmsv1.InstanceOperation_CHECK_IF_PRIMARY,
								},
							},
						},
					}, nil)
				meta.EXPECT().GetClusterByInstanceID(gomock.Any(), testInstanceID, "test-cluster").Return(
					models.Cluster{ID: "test-cluster"},
					nil,
				)
				return fields{
					cms:  cms,
					meta: meta,
				}
			},
			args: args{
				duration:    time.Minute,
				clusterType: "test-cluster",
			},
			want: []models.Cluster{
				{ID: "test-cluster", TargetMaintenanceVtypeID: "test-instance"},
			},
		},
		{
			name: "cluster type not in metadb",
			fields: func(ctrl *gomock.Controller) fields {
				testInstanceID := "test-instance"
				meta := mockmeta.NewMockMetaDB(ctrl)
				cms := mockcms.NewMockInstanceClient(ctrl)
				cms.EXPECT().List(gomock.Any()).Return(
					&cmsv1.ListInstanceOperationsResponse{
						Operations: []*cmsv1.InstanceOperation{
							{
								InstanceId: testInstanceID,
								CreatedAt:  timestamppb.New(time.Now().Add(-10 * time.Minute)),
								ExecutedSteps: []cmsv1.InstanceOperation_StepName{
									cmsv1.InstanceOperation_CHECK_IF_PRIMARY,
								},
							},
						},
					}, nil)
				meta.EXPECT().GetClusterByInstanceID(gomock.Any(), testInstanceID, "test-cluster").Return(
					models.Cluster{},
					sqlerrors.ErrNotFound,
				)
				return fields{
					cms:  cms,
					meta: meta,
				}
			},
			args: args{
				duration:    time.Minute,
				clusterType: "test-cluster",
			},
			want: []models.Cluster(nil),
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			fieldsContent := tt.fields(ctrl)
			g := &GRPCCMS{
				cms:  fieldsContent.cms,
				meta: fieldsContent.meta,
			}
			got, err := g.AwaitingPrimarySwitchover(ctx, tt.args.duration, tt.args.clusterType)
			require.NoError(t, err)
			require.Equal(t, tt.want, got)
			ctrl.Finish()
		})
	}
}
