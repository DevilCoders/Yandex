package implementation

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/conductorcache"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/dbm/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestDom0DiscoveryImpl_Dom0Instances(t *testing.T) {
	type fields struct {
		dbm          dbm.Client
		L            log.Logger
		ccache       *conductorcache.Cache
		wellKnownG   map[string]bool
		blacklistedG map[string]bool
	}
	type args struct {
		ctx  context.Context
		dom0 string
	}
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	ctx := context.Background()
	testErr := xerrors.NewSentinel("test error")
	dom0fqdn := "dom0.y.net"
	i1 := "instance1.y.net"
	spaceLimit := 1230200329
	//i2 := "instance2.y.net"
	tests := []struct {
		name     string
		fields   func(dbmcl *mocks.MockClient) fields
		args     args
		wantCtrs dom0discovery.DiscoveryResult
		wantErr  error
	}{
		{
			name: "empty dom0",
			fields: func(dbmcl *mocks.MockClient) fields {
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), dom0fqdn).Return(
					[]dbm.Container{}, nil)
				return fields{
					dbm: dbmcl,
					L:   logger,
				}
			},
			args: args{
				ctx:  ctx,
				dom0: dom0fqdn,
			},
			wantCtrs: dom0discovery.DiscoveryResult{
				WellKnown: []models.Instance{},
				Unknown:   []string{},
			},
			wantErr: nil,
		},
		{
			name: "dbm.Dom0Containers error",
			fields: func(dbmcl *mocks.MockClient) fields {
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), dom0fqdn).Return(
					[]dbm.Container{}, testErr)
				return fields{
					dbm: dbmcl,
					L:   logger,
				}
			},
			args: args{
				ctx:  ctx,
				dom0: dom0fqdn,
			},
			wantCtrs: dom0discovery.DiscoveryResult{},
			wantErr:  testErr,
		},
		{
			name: "dbm.VolumesByDom0 error",
			fields: func(dbmcl *mocks.MockClient) fields {
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), dom0fqdn).Return(
					[]dbm.Container{
						{FQDN: i1},
					}, nil)
				dbmcl.EXPECT().VolumesByDom0(gomock.Any(), dom0fqdn).Return(
					map[string]dbm.ContainerVolumes{}, testErr)
				return fields{
					dbm: dbmcl,
					L:   logger,
				}
			},
			args: args{
				ctx:  ctx,
				dom0: dom0fqdn,
			},
			wantCtrs: dom0discovery.DiscoveryResult{},
			wantErr:  testErr,
		},
		{
			name: "one known with parent",
			fields: func(dbmcl *mocks.MockClient) fields {
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), dom0fqdn).Return(
					[]dbm.Container{
						{FQDN: i1},
					}, nil)
				dbmcl.EXPECT().VolumesByDom0(gomock.Any(), dom0fqdn).Return(
					map[string]dbm.ContainerVolumes{
						i1: {
							FQDN:    i1,
							Volumes: []dbm.Volume{{SpaceLimit: spaceLimit}},
						},
					}, nil)
				ccache := conductorcache.NewCache()
				ccache.UpdateHosts(map[string][]string{
					i1: {"test-group", "parent-test-group"},
				})

				return fields{
					dbm:    dbmcl,
					ccache: ccache,
					L:      logger,
					wellKnownG: map[string]bool{
						"parent-test-group": true,
					},
				}
			},
			args: args{
				ctx:  ctx,
				dom0: dom0fqdn,
			},
			wantCtrs: dom0discovery.DiscoveryResult{
				WellKnown: []models.Instance{{
					FQDN:           i1,
					ConductorGroup: "test-group",
					Volumes:        []dbm.Volume{{SpaceLimit: spaceLimit}},
				}},
				Unknown: []string{},
			},
			wantErr: nil,
		},
		{
			name: "one blacklisted",
			fields: func(dbmcl *mocks.MockClient) fields {
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), dom0fqdn).Return(
					[]dbm.Container{
						{FQDN: i1},
					}, nil)
				dbmcl.EXPECT().VolumesByDom0(gomock.Any(), dom0fqdn).Return(
					map[string]dbm.ContainerVolumes{
						i1: {
							FQDN: i1,
						},
					}, nil)
				ccache := conductorcache.NewCache()
				ccache.UpdateHosts(map[string][]string{
					i1: {"blacklist-test-g"},
				})

				return fields{
					dbm:        dbmcl,
					ccache:     ccache,
					L:          logger,
					wellKnownG: map[string]bool{},
					blacklistedG: map[string]bool{
						"blacklist-test-g": true,
					},
				}
			},
			args: args{
				ctx:  ctx,
				dom0: dom0fqdn,
			},
			wantCtrs: dom0discovery.DiscoveryResult{
				WellKnown: []models.Instance{},
				Unknown:   []string{},
			},
			wantErr: nil,
		},
		{
			name: "one unknown with no parents",
			fields: func(dbmcl *mocks.MockClient) fields {
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), dom0fqdn).Return(
					[]dbm.Container{
						{FQDN: i1},
					}, nil)
				dbmcl.EXPECT().VolumesByDom0(gomock.Any(), dom0fqdn).Return(
					map[string]dbm.ContainerVolumes{
						i1: {
							FQDN: i1,
						},
					}, nil)
				ccache := conductorcache.NewCache()
				ccache.UpdateHosts(map[string][]string{
					i1: {"test-group"},
				})

				return fields{
					dbm:        dbmcl,
					ccache:     ccache,
					L:          logger,
					wellKnownG: map[string]bool{"some-test-group": true},
				}
			},
			args: args{
				ctx:  ctx,
				dom0: dom0fqdn,
			},
			wantCtrs: dom0discovery.DiscoveryResult{
				WellKnown: []models.Instance{},
				Unknown:   []string{i1},
			},
			wantErr: nil,
		},
		{
			name: "one known without volume info",
			fields: func(dbmcl *mocks.MockClient) fields {
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), dom0fqdn).Return(
					[]dbm.Container{
						{FQDN: i1},
					}, nil)
				dbmcl.EXPECT().VolumesByDom0(gomock.Any(), dom0fqdn).Return(
					map[string]dbm.ContainerVolumes{}, nil)
				ccache := conductorcache.NewCache()
				ccache.UpdateHosts(map[string][]string{
					i1: {"test-group", "parent-test-group"},
				})

				return fields{
					dbm:    dbmcl,
					ccache: ccache,
					L:      logger,
					wellKnownG: map[string]bool{
						"parent-test-group": true,
					},
				}
			},
			args: args{
				ctx:  ctx,
				dom0: dom0fqdn,
			},
			wantCtrs: dom0discovery.DiscoveryResult{
				WellKnown: []models.Instance{{FQDN: i1, ConductorGroup: "test-group", Volumes: []dbm.Volume{}}},
				Unknown:   []string{},
			},
			wantErr: nil,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			dcl := mocks.NewMockClient(ctrl)
			fields := tt.fields(dcl)
			d := &Dom0DiscoveryImpl{
				dbm:     fields.dbm,
				cache:   fields.ccache,
				L:       fields.L,
				whiteLG: fields.wellKnownG,
				blackLG: fields.blacklistedG,
			}
			gotCtrs, err := d.Dom0Instances(tt.args.ctx, tt.args.dom0)
			if tt.wantErr != nil {
				require.ErrorIs(t, err, tt.wantErr)
				return
			}
			require.NoError(t, err)
			require.Equal(t, tt.wantCtrs, gotCtrs)
		})
	}
}
