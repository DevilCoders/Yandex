package jugglerbased

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	mocks2 "a.yandex-team.ru/cloud/mdb/internal/conductor/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestJugglerBasedHealthiness_ByInstances(t *testing.T) {
	type fields struct {
		cncl          conductor.Client
		knownGroupMap []models.KnownGroups
		jChkr         juggler.JugglerChecker
	}
	ctx := context.Background()
	type args struct {
		instances []models.Instance
	}
	iToTest := models.Instance{
		FQDN: "test-fqdn",
	}
	tests := []struct {
		name    string
		fields  func(ctrl *gomock.Controller) fields
		args    args
		want    healthiness.HealthCheckResult
		wantErr error
	}{
		{
			name: "unknown group",
			fields: func(ctrl *gomock.Controller) fields {
				ccl := mocks2.NewMockClient(ctrl)
				ccl.EXPECT().HostsInfo(gomock.Any(), []string{iToTest.FQDN}).Return([]conductor.HostInfo{
					{
						Group: "test-group",
						FQDN:  iToTest.FQDN,
					},
				}, nil)
				ccl.EXPECT().GroupInfoByName(gomock.Any(), "test-group").Return(conductor.GroupInfo{
					Name: "test-group",
				}, nil)
				ccl.EXPECT().ParentGroup(gomock.Any(), gomock.Any()).Return(conductor.GroupInfo{}, semerr.NotFound("test-error"))
				return fields{
					cncl:          ccl,
					knownGroupMap: []models.KnownGroups{},
					jChkr:         nil,
				}
			},
			args: args{
				instances: []models.Instance{iToTest},
			},
			want: healthiness.HealthCheckResult{
				Unknown: []healthiness.FQDNCheck{
					{
						Instance:        iToTest,
						Cid:             NotDbaas,
						Sid:             NotDbaas,
						Roles:           nil,
						StatusUpdatedAt: time.Time{},
						HintIs:          "no suitable conductor group in config exists, choose one and add it to config",
					},
				},
			},
			wantErr: nil,
		},
		{
			name: "will degrade",
			fields: func(ctrl *gomock.Controller) fields {
				ccl := mocks2.NewMockClient(ctrl)
				ccl.EXPECT().HostsInfo(gomock.Any(), []string{iToTest.FQDN}).Return([]conductor.HostInfo{
					{
						Group: "test-group",
						FQDN:  iToTest.FQDN,
					},
				}, nil)
				ccl.EXPECT().GroupToHosts(gomock.Any(), iToTest.ConductorGroup, conductor.GroupToHostsAttrs{}).Return([]string{
					iToTest.FQDN,
					"test-host2",
				}, nil)
				jg := mocks.NewMockJugglerChecker(ctrl)
				jg.EXPECT().Check(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(juggler.FQDNGroupByJugglerCheck{
					OK: []string{},
					NotOK: []string{
						"test-host2",
					},
				}, nil)
				return fields{
					cncl: ccl,
					knownGroupMap: []models.KnownGroups{
						{
							Type:         "test group container",
							CGroups:      []string{"test-group"},
							Checks:       []string{"test-check"},
							MaxUnhealthy: 1,
						},
					},
					jChkr: jg,
				}
			},
			args: args{
				instances: []models.Instance{iToTest},
			},
			want: healthiness.HealthCheckResult{
				WouldDegrade: []healthiness.FQDNCheck{
					{
						Instance:        iToTest,
						Cid:             NotDbaas,
						Sid:             NotDbaas,
						Roles:           []string{"test group container"},
						StatusUpdatedAt: time.Time{},
						HintIs:          "1 already degraded",
					},
				},
			},
			wantErr: nil,
		},
		{
			name: "give away",
			fields: func(ctrl *gomock.Controller) fields {
				ccl := mocks2.NewMockClient(ctrl)
				ccl.EXPECT().HostsInfo(gomock.Any(), []string{iToTest.FQDN}).Return([]conductor.HostInfo{
					{
						Group: "test-group",
						FQDN:  iToTest.FQDN,
					},
				}, nil)
				ccl.EXPECT().GroupToHosts(gomock.Any(), iToTest.ConductorGroup, conductor.GroupToHostsAttrs{}).Return([]string{
					iToTest.FQDN,
					"test-host2",
				}, nil)
				jg := mocks.NewMockJugglerChecker(ctrl)
				jg.EXPECT().Check(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(juggler.FQDNGroupByJugglerCheck{
					OK:    []string{},
					NotOK: []string{},
				}, nil)
				return fields{
					cncl: ccl,
					knownGroupMap: []models.KnownGroups{
						{
							Type:         "test group container",
							CGroups:      []string{"test-group"},
							Checks:       []string{"test-check"},
							MaxUnhealthy: 1,
						},
					},
					jChkr: jg,
				}
			},
			args: args{
				instances: []models.Instance{iToTest},
			},
			want: healthiness.HealthCheckResult{
				GiveAway: []healthiness.FQDNCheck{
					{
						Instance:        iToTest,
						Cid:             NotDbaas,
						Sid:             NotDbaas,
						Roles:           []string{"test group container"},
						StatusUpdatedAt: time.Time{},
						HintIs:          "",
					},
				},
			},
			wantErr: nil,
		},
		{
			name: "ignored",
			fields: func(ctrl *gomock.Controller) fields {
				ccl := mocks2.NewMockClient(ctrl)
				ccl.EXPECT().HostsInfo(gomock.Any(), []string{iToTest.FQDN}).Return([]conductor.HostInfo{
					{
						Group: "test-group",
						FQDN:  iToTest.FQDN,
					},
				}, nil)
				return fields{
					cncl: ccl,
					knownGroupMap: []models.KnownGroups{
						{
							Type:         "test group container",
							CGroups:      []string{"test-group"},
							Checks:       []string{"test-check"},
							MaxUnhealthy: 0,
						},
					},
					jChkr: nil,
				}
			},
			args: args{
				instances: []models.Instance{iToTest},
			},
			want: healthiness.HealthCheckResult{
				Unknown: []healthiness.FQDNCheck{
					{
						Instance:        iToTest,
						Cid:             "",
						Sid:             "",
						Roles:           nil,
						StatusUpdatedAt: time.Time{},
						HintIs:          "marked unknown, cause MaxUnhealthy is zero",
					},
				},
			},
			wantErr: nil,
		},
	}
	logger, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			f := tt.fields(ctrl)
			h := NewJugglerBasedHealthiness(f.cncl, f.knownGroupMap, f.jChkr, logger)
			got, err := h.ByInstances(ctx, tt.args.instances)
			if tt.wantErr != nil {
				require.Error(t, err)
				require.ErrorIs(t, err, tt.wantErr)
			} else {
				require.NoError(t, err)
				require.Equal(t, tt.want, got)
			}
		})
	}
}
