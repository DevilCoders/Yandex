package healthbased

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/healthbased/healthdbspec"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	mocks2 "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	types2 "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func TestHealthBasedHealthiness_ByInstances(t *testing.T) {
	type fields struct {
		health client.MDBHealthClient
		rr     healthdbspec.RoleSpecificResolvers
	}
	ctx := context.Background()
	type args struct {
		instances []models.Instance
	}
	iUnknown := models.Instance{
		FQDN: "test-unknown",
	}
	iStale := models.Instance{
		FQDN: "test-stale",
	}
	iOK := models.Instance{
		FQDN: "test-ok",
	}
	iUnhealthy := models.Instance{
		FQDN: "test-ok",
	}
	iNonProd := models.Instance{
		FQDN: "test-nonProd",
	}
	testDate := time.Date(2020, 5, 1, 0, 0, 0, 0, time.Local)
	tests := []struct {
		name    string
		fields  func(ctrl *gomock.Controller) fields
		args    args
		wantRes healthiness.HealthCheckResult
		wantErr error
	}{
		{
			name: "unknown",
			wantRes: healthiness.HealthCheckResult{
				Unknown: []healthiness.FQDNCheck{
					{
						Instance: iUnknown,
					},
				},
			},
			fields: func(ctrl *gomock.Controller) fields {
				health := mocks2.NewMockMDBHealthClient(ctrl)
				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), gomock.Any()).Return(map[string]types2.HostNeighboursInfo{}, nil)
				return fields{
					health: health,
					rr:     nil,
				}
			},
			args: args{instances: []models.Instance{iUnknown}},
		},
		{
			name: "ignored",
			wantRes: healthiness.HealthCheckResult{
				Ignored: []healthiness.FQDNCheck{
					{
						Instance:        iNonProd,
						Roles:           []string{"test-role"},
						CntTotalInGroup: 1,
					},
				},
			},
			fields: func(ctrl *gomock.Controller) fields {
				health := mocks2.NewMockMDBHealthClient(ctrl)
				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), gomock.Any()).Return(map[string]types2.HostNeighboursInfo{
					iNonProd.FQDN: {
						Env:            ProdEnvName,
						SameRolesTotal: 0,
						Roles:          []string{"test-role"},
					},
				}, nil)
				rr := healthdbspec.RoleSpecificResolvers{}
				rr = append(rr, healthdbspec.RoleResolver{
					RoleMatch: func(s string) bool {
						return true
					},
					OkToLetGo: func(_ types2.HostNeighboursInfo) bool {
						return false
					},
				})
				return fields{
					health: health,
					rr:     rr,
				}
			},
			args: args{instances: []models.Instance{iNonProd}},
		},
		{
			name: "stale",
			wantRes: healthiness.HealthCheckResult{
				Stale: []healthiness.FQDNCheck{
					{
						Instance:        iStale,
						HAShard:         true,
						HACluster:       true,
						CntTotalInGroup: 1,
						StatusUpdatedAt: testDate.Add(-time.Hour),
						HintIs:          "stale for 1h0m0s",
					},
				},
			},
			fields: func(ctrl *gomock.Controller) fields {
				health := mocks2.NewMockMDBHealthClient(ctrl)
				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), gomock.Any()).Return(map[string]types2.HostNeighboursInfo{
					iStale.FQDN: {
						SameRolesTS: testDate.Add(-time.Hour),
						HACluster:   true,
						HAShard:     true,
					},
				}, nil)
				rr := healthdbspec.RoleSpecificResolvers{}
				return fields{
					health: health,
					rr:     rr,
				}
			},
			args: args{instances: []models.Instance{iStale}},
		}, {
			name: "ok",
			wantRes: healthiness.HealthCheckResult{
				GiveAway: []healthiness.FQDNCheck{
					{
						Instance:        iOK,
						HACluster:       true,
						HAShard:         true,
						CntTotalInGroup: 1,
						StatusUpdatedAt: testDate,
					},
				},
			},
			fields: func(ctrl *gomock.Controller) fields {
				health := mocks2.NewMockMDBHealthClient(ctrl)
				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), gomock.Any()).Return(map[string]types2.HostNeighboursInfo{
					iOK.FQDN: {
						SameRolesTS: testDate,
						HACluster:   true,
						HAShard:     true,
					},
				}, nil)
				rr := healthdbspec.RoleSpecificResolvers{}
				return fields{
					health: health,
					rr:     rr,
				}
			},
			args: args{instances: []models.Instance{iOK}},
		}, {
			name: "degrades",
			wantRes: healthiness.HealthCheckResult{
				WouldDegrade: []healthiness.FQDNCheck{
					{
						Instance:        iUnhealthy,
						StatusUpdatedAt: testDate,
						HAShard:         true,
						HACluster:       true,
						CntTotalInGroup: 1,
						Roles:           []string{"test"},
					},
				},
			},
			fields: func(ctrl *gomock.Controller) fields {
				health := mocks2.NewMockMDBHealthClient(ctrl)
				health.EXPECT().GetHostNeighboursInfo(gomock.Any(), gomock.Any()).Return(map[string]types2.HostNeighboursInfo{
					iOK.FQDN: {
						SameRolesTS: testDate,
						HACluster:   true,
						HAShard:     true,
						Roles:       []string{"test"},
					},
				}, nil)
				rr := healthdbspec.RoleSpecificResolvers{}
				rr = append(rr, healthdbspec.RoleResolver{
					RoleMatch: func(s string) bool {
						return true
					},
					OkToLetGo: func(_ types2.HostNeighboursInfo) bool {
						return false
					},
				})
				return fields{
					health: health,
					rr:     rr,
				}
			},
			args: args{instances: []models.Instance{iUnhealthy}},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			fields := tt.fields(ctrl)
			h := NewHealthBasedHealthiness(fields.health, fields.rr, testDate)
			gotRes, err := h.ByInstances(ctx, tt.args.instances)
			if tt.wantErr != nil {
				require.Error(t, err)
				return
			} else {
				require.NoError(t, err)
				require.Equal(t, tt.wantRes, gotRes)
			}
		})
	}
}
