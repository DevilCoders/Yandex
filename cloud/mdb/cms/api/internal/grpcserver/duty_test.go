package grpcserver

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	cmsdbmocks "a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/conductorcache"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/push"
	pushmocks "a.yandex-team.ru/cloud/mdb/internal/juggler/push/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestAlarm(t *testing.T) {
	const (
		serverFQDN = "cmsfqdn"
	)

	testCases := []struct {
		name      string
		stale     []models.ManagementInstanceOperation
		alarm     []models.ManagementInstanceOperation
		events    []push.Event
		ccache    map[string][]string
		pusherErr error
		res       *api.AlarmResponse
	}{
		{
			name: "full house",
			stale: []models.ManagementInstanceOperation{{
				ID:     "qwe",
				Status: models.InstanceOperationStatusInProgress,
				State:  &models.OperationState{FQDN: "fqdn1.a"},
			}},
			alarm: []models.ManagementInstanceOperation{
				{
					ID:     "asd",
					Status: models.InstanceOperationStatusRejected,
					State:  &models.OperationState{FQDN: "fqdn2.b"},
				},
				{
					ID:     "zxc",
					Status: models.InstanceOperationStatusRejected,
					State:  &models.OperationState{},
				},
			},
			events: []push.Event{
				{
					Host:        "fqdn1.b",
					Service:     staleService,
					Status:      push.CRIT,
					Description: "https:///cms/instanceoperation/qwe",
				},
				{
					Host:        "fqdn2.b",
					Service:     rejectService,
					Status:      push.CRIT,
					Description: "https:///cms/instanceoperation/asd",
				},
				{
					Host:        serverFQDN,
					Service:     unknownService,
					Status:      push.CRIT,
					Description: "https:///cms/instanceoperation/zxc",
				},
			},
			ccache: map[string][]string{"fqdn1.b": nil, "fqdn2.b": nil},
			res: &api.AlarmResponse{
				Status:      api.AlarmResponse_OK,
				Description: "OK"},
		},
		{
			name: "everything is OK",
			res:  &api.AlarmResponse{Status: api.AlarmResponse_OK, Description: "OK"},
		},
		{
			name: "only one stale",
			stale: []models.ManagementInstanceOperation{{
				ID:     "qwe",
				Status: models.InstanceOperationStatusInProgress,
				State:  &models.OperationState{FQDN: "fqdn1.a"},
			}},
			events: []push.Event{
				{
					Host:        "fqdn1.b",
					Service:     staleService,
					Status:      push.CRIT,
					Description: "https:///cms/instanceoperation/qwe",
				},
				{
					Host:        serverFQDN,
					Service:     unknownService,
					Status:      push.OK,
					Description: "OK",
				},
			},
			ccache: map[string][]string{"fqdn1.b": nil},
			res: &api.AlarmResponse{
				Status:      api.AlarmResponse_OK,
				Description: "OK"},
		},
		{
			name: "one unknown",
			stale: []models.ManagementInstanceOperation{{
				ID:     "qwe",
				Status: models.InstanceOperationStatusInProgress,
				State:  &models.OperationState{FQDN: "fqdn1.a"},
			}},
			events: []push.Event{
				{
					Host:        serverFQDN,
					Service:     unknownService,
					Status:      push.CRIT,
					Description: "https:///cms/instanceoperation/qwe",
				},
			},
			res: &api.AlarmResponse{
				Status:      api.AlarmResponse_OK,
				Description: "OK"},
		},
		{
			name: "two unknown",
			stale: []models.ManagementInstanceOperation{{
				ID:     "qwe",
				Status: models.InstanceOperationStatusInProgress,
				State:  &models.OperationState{FQDN: "fqdn1.a"},
			}},
			alarm: []models.ManagementInstanceOperation{{
				ID:     "asd",
				Status: models.InstanceOperationStatusRejected,
				State:  &models.OperationState{FQDN: "fqdn2.a"},
			}},
			events: []push.Event{
				{
					Host:        serverFQDN,
					Service:     unknownService,
					Status:      push.CRIT,
					Description: "https:///cms/instanceoperation/qwe, https:///cms/instanceoperation/asd",
				},
			},
			res: &api.AlarmResponse{
				Status:      api.AlarmResponse_OK,
				Description: "OK"},
		},
		{
			name: "pusher error",
			stale: []models.ManagementInstanceOperation{{
				ID:     "qwe",
				Status: models.InstanceOperationStatusInProgress,
				State:  &models.OperationState{FQDN: "fqdn1.a"},
			}},
			events: []push.Event{
				{
					Host:        serverFQDN,
					Service:     unknownService,
					Status:      push.CRIT,
					Description: "https:///cms/instanceoperation/qwe",
				},
			},
			pusherErr: xerrors.New("test error"),
			res: &api.AlarmResponse{
				Status:      api.AlarmResponse_CRITICAL,
				Description: "can not send events: test error"},
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()

			cmsdb := cmsdbmocks.NewMockClient(ctrl)
			cmsdb.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(ctx, nil)
			cmsdb.EXPECT().Rollback(gomock.Any()).Return(nil)
			cmsdb.EXPECT().StaleInstanceOperations(gomock.Any()).Return(tc.stale, nil)
			cmsdb.EXPECT().InstanceOperationsToAlarm(gomock.Any()).Return(tc.alarm, nil)

			pusher := pushmocks.NewMockPusher(ctrl)
			if len(tc.events) > 0 {
				pusher.EXPECT().Push(gomock.Any(), gomock.Any()).DoAndReturn(func(ctx context.Context, request push.Request) error {
					assert.Equal(t, tc.events, request.Events)
					return tc.pusherErr
				})
			}

			ccache := conductorcache.NewCache()
			ccache.UpdateHosts(tc.ccache)

			server := NewInstanceService(cmsdb, nil, nil, pusher, nil, nil, nil, ccache,
				&Config{FQDN: serverFQDN, FQDNSuffixes: FQDNSuffixes{
					UnmanagedDataplane: "a",
					ManagedDataplane:   "b",
				}})
			res, err := server.Alarm(ctx, nil)
			assert.NoError(t, err)
			assert.Equal(t, tc.res.Status, res.Status)
			assert.Equal(t, tc.res.Description, res.Description)
		})
	}
}
