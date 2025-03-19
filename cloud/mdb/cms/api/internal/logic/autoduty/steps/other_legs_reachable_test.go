package steps

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	juggler2 "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglermock "a.yandex-team.ru/cloud/mdb/internal/juggler/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
)

func TestWaitOtherLegsReachableStep_checkOtherLegsReachable(t *testing.T) {
	type fields struct {
		action       AfterStepAction
		reachability juggler2.JugglerChecker
		dom0d        dom0discovery.Dom0Discovery
		mDB          metadb.MetaDB
	}
	ctx := context.Background()
	now := time.Now()
	type args struct {
		currentFQDNs []string
	}
	tests := []struct {
		name    string
		fields  func(ctrl *gomock.Controller) fields
		args    args
		want    UnreachableLegsMap
		wantErr bool
	}{
		{
			name:    "1 reachable, 1 not",
			wantErr: false,
			want: UnreachableLegsMap{
				unreachable: map[string][]string{
					"foo.y.net": {"unreach.y.net"},
				},
			},
			args: args{currentFQDNs: []string{"foo.y.net"}},
			fields: func(ctrl *gomock.Controller) fields {
				meta := metadbmocks.NewMockMetaDB(ctrl)
				meta.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(ctx, nil)
				meta.EXPECT().Rollback(gomock.Any()).Return(nil)
				meta.EXPECT().Commit(gomock.Any()).Return(nil)
				meta.EXPECT().GetHostByFQDN(gomock.Any(), "foo.y.net").Return(metadb.Host{FQDN: "foo.y.net", SubClusterID: "qwe"}, nil)
				meta.EXPECT().GetHostsBySubcid(gomock.Any(), "qwe").Return([]metadb.Host{
					{
						FQDN:         "foo.y.net",
						SubClusterID: "qwe",
						Geo:          "dc0",
						Roles:        []string{"role"},
					},
					{
						FQDN:         "reach.y.net",
						SubClusterID: "qwe",
						Geo:          "dc1",
						Roles:        []string{"role"},
					},
					{
						FQDN:         "unreach.y.net",
						SubClusterID: "qwe",
						Geo:          "dc2",
						Roles:        []string{"role"},
					},
				}, nil)
				jglr := jugglermock.NewMockAPI(ctrl)
				jglr.EXPECT().RawEvents(ctx, []string{"reach.y.net", "unreach.y.net"}, []string{"UNREACHABLE"}).Return([]juggler.RawEvent{
					{
						Host:         "reach.y.net",
						Service:      "UNREACHABLE",
						Status:       "OK",
						ReceivedTime: now,
					},
					{
						Host:         "unreach.y.net",
						Service:      "UNREACHABLE",
						Status:       "CRIT",
						ReceivedTime: now,
					}}, nil)
				rcheck := juggler2.NewJugglerReachabilityChecker(jglr, 10)
				return fields{
					action:       AfterStepWait,
					reachability: rcheck,
					dom0d:        nil,
					mDB:          meta,
				}
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			testFields := tt.fields(ctrl)
			s := &WaitOtherLegsReachableStep{
				reachability: testFields.reachability,
				dom0d:        testFields.dom0d,
				mDB:          testFields.mDB,
			}
			got, err := s.checkOtherLegsReachable(ctx, tt.args.currentFQDNs)
			if tt.wantErr {
				require.Error(t, err)
			} else {
				require.NoError(t, err)
			}
			require.Equal(t, tt.want, got)
		})
	}
}

func Test_fmtUnreachableLegsMapResult(t *testing.T) {
	type args struct {
		ulm UnreachableLegsMap
	}
	tests := []struct {
		name string
		args args
		want RunResult
	}{
		{
			name: "unknown",
			args: args{
				ulm: UnreachableLegsMap{
					unreachable: map[string][]string{},
					unknown: []string{
						"unkn.y.net",
					},
				},
			},
			want: RunResult{
				ForHuman:     "1 legs skipped, they are not in metadb, so they have no legs: unkn.y.net\nAll known clusters reachable",
				Action:       AfterStepContinue,
				Error:        nil,
				AfterMeSteps: nil,
			},
		}, {
			name: "unreachable",
			args: args{
				ulm: UnreachableLegsMap{
					unreachable: map[string][]string{
						"foo.y.net": {"bar.y.net", "baz.y.net"},
					},
					unknown: []string{},
				},
			},
			want: RunResult{
				ForHuman:     "NotOK legs exist, you should bring them up:\n  * for 'foo.y.net' unreachable: bar.y.net, baz.y.net",
				Action:       AfterStepWait,
				Error:        nil,
				AfterMeSteps: nil,
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, fmtUnreachableLegsMapResult(tt.args.ulm))
		})
	}
}
