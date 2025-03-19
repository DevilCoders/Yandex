package logic

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/jonboulle/clockwork"
	"github.com/stretchr/testify/require"

	cmsmock "a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/duty"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	dbmmock "a.yandex-team.ru/cloud/mdb/internal/dbm/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/push"
	pushermock "a.yandex-team.ru/cloud/mdb/internal/juggler/push/mocks"
)

var (
	notFridayNow = time.Date(2021, 10, 21, 12, 00, 00, 00, time.Local)
	fridayNow    = time.Date(2021, 10, 22, 12, 00, 00, 00, time.Local)
	sundayNow    = time.Date(2021, 10, 24, 12, 00, 00, 00, time.Local)
)

const (
	OkHoursAtWalle           = 24
	OkFridayHoursAtWalle     = 2
	OkHoursUnfinished        = 3
	DutyDecisionThresholdMin = 10
)

func newManagementRequest(id int64, fqdn string, ageMinutes uint, now time.Time) models.ManagementRequest {
	return models.ManagementRequest{
		ID:         id,
		Name:       "profile",
		ExtID:      fmt.Sprintf("ext-id-%d", id),
		Status:     "in-process",
		Fqnds:      []string{fqdn},
		ResolvedAt: now.Add(-time.Duration(ageMinutes) * time.Minute),
	}
}

func makeMonrunInteractor(
	t *testing.T,
	now time.Time,
	mockCMSDB func(cmsdb *cmsmock.MockClient, now time.Time),
	mockDBM func(dbmcl *dbmmock.MockClient),
	mockPusher func(pusher *pushermock.MockPusher, t *testing.T),
) (MonrunInteractor, *gomock.Controller) {
	cfg := duty.CmsDom0DutyConfig{
		OkHoursAtWalle:           OkHoursAtWalle,
		OkFridayHoursAtWalle:     OkFridayHoursAtWalle,
		OkHoursUnfinished:        OkHoursUnfinished,
		DutyDecisionThresholdMin: DutyDecisionThresholdMin,
		UIHost:                   "ui",
	}
	ctrl := gomock.NewController(t)

	cmsdb := cmsmock.NewMockClient(ctrl)
	mockCMSDB(cmsdb, now)

	dbmcl := dbmmock.NewMockClient(ctrl)
	mockDBM(dbmcl)

	pusher := pushermock.NewMockPusher(ctrl)
	if mockPusher != nil {
		mockPusher(pusher, t)
	}

	mr := NewMonrunInteractor(nil, cmsdb, cfg, dbmcl, pusher)
	mr.clock = clockwork.NewFakeClockAt(now)
	return mr, ctrl
}

func TestSendEvents(t *testing.T) {
	tcs := []struct {
		name       string
		now        time.Time
		mockCMSDB  func(cmsdb *cmsmock.MockClient, now time.Time)
		mockDBM    func(dbmcl *dbmmock.MockClient)
		mockPusher func(pusher *pushermock.MockPusher, t *testing.T)
	}{
		{
			name: "friday",
			now:  fridayNow,
			mockCMSDB: func(cmsdb *cmsmock.MockClient, now time.Time) {
				cmsdb.EXPECT().GetRequestsToConsider(gomock.Any(), DutyDecisionThresholdMin).Return(
					[]models.ManagementRequest{
						newManagementRequest(101, "toConsiderEmpty", 20, now),
						newManagementRequest(102, "toConsiderDom0", 22, now),
					}, nil,
				)
				cmsdb.EXPECT().GetUnfinishedRequests(gomock.Any(), time.Hour*OkHoursUnfinished).Return(
					[]models.ManagementRequest{
						newManagementRequest(105, "unfinishedEmpty", 3*60+1, now),
						newManagementRequest(106, "unfinishedDom0", 4*60, now),
					}, nil,
				)
				cmsdb.EXPECT().GetResetupRequests(gomock.Any(), time.Hour*OkFridayHoursAtWalle).Return(
					[]models.ManagementRequest{
						newManagementRequest(107, "resetupEmpty", 3*60, now),
						newManagementRequest(108, "resetupDom0", 2*60+1, now),
					}, nil,
				)
			},
			mockDBM: func(dbmcl *dbmmock.MockClient) {
				empty := make([]dbm.Container, 0)
				dom0 := append(empty, dbm.Container{})
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), "resetupEmpty").Return(empty, nil)
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), "resetupDom0").Return(dom0, nil)
			},
			mockPusher: func(pusher *pushermock.MockPusher, t *testing.T) {
				pusher.EXPECT().Push(gomock.Any(), gomock.Any()).DoAndReturn(func(ctx context.Context, request push.Request) error {
					require.Equal(t, []push.Event{
						{
							Host:        "toConsiderEmpty",
							Service:     "cms_consider_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-101",
						},
						{
							Host:        "toConsiderDom0",
							Service:     "cms_consider_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-102",
						},
						{
							Host:        "unfinishedEmpty",
							Service:     "cms_unfinished_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-105",
						},
						{
							Host:        "unfinishedDom0",
							Service:     "cms_unfinished_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-106",
						},
						{
							Host:        "resetupDom0",
							Service:     "cms_resetup_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-108",
						},
					}, request.Events)
					return nil
				})
			},
		},
		{
			name: "sunday",
			now:  sundayNow,
			mockCMSDB: func(cmsdb *cmsmock.MockClient, now time.Time) {
				cmsdb.EXPECT().GetRequestsToConsider(gomock.Any(), DutyDecisionThresholdMin).Return(
					[]models.ManagementRequest{
						newManagementRequest(101, "toConsiderEmpty", 20, now),
						newManagementRequest(102, "toConsiderDom0", 22, now),
					}, nil,
				)
				cmsdb.EXPECT().GetUnfinishedRequests(gomock.Any(), time.Hour*OkHoursUnfinished).Return(
					[]models.ManagementRequest{
						newManagementRequest(105, "unfinishedEmpty", 3*60+1, now),
						newManagementRequest(106, "unfinishedDom0", 4*60, now),
					}, nil,
				)
				cmsdb.EXPECT().GetResetupRequests(gomock.Any(), time.Hour*OkFridayHoursAtWalle).Return(
					[]models.ManagementRequest{
						newManagementRequest(107, "resetupEmpty", 3*60, now),
						newManagementRequest(108, "resetupDom0", 2*60+1, now),
					}, nil,
				)
			},
			mockDBM: func(dbmcl *dbmmock.MockClient) {
				empty := make([]dbm.Container, 0)
				dom0 := append(empty, dbm.Container{})
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), "resetupEmpty").Return(empty, nil)
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), "resetupDom0").Return(dom0, nil)
			},
			mockPusher: func(pusher *pushermock.MockPusher, t *testing.T) {
				pusher.EXPECT().Push(gomock.Any(), gomock.Any()).DoAndReturn(func(ctx context.Context, request push.Request) error {
					require.Equal(t, []push.Event{
						{
							Host:        "toConsiderEmpty",
							Service:     "cms_consider_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-101",
						},
						{
							Host:        "toConsiderDom0",
							Service:     "cms_consider_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-102",
						},
						{
							Host:        "unfinishedEmpty",
							Service:     "cms_unfinished_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-105",
						},
						{
							Host:        "unfinishedDom0",
							Service:     "cms_unfinished_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-106",
						},
						{
							Host:        "resetupDom0",
							Service:     "cms_resetup_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-108",
						},
					}, request.Events)
					return nil
				})
			},
		},
		{
			name: "not friday",
			now:  notFridayNow,
			mockCMSDB: func(cmsdb *cmsmock.MockClient, now time.Time) {
				cmsdb.EXPECT().GetRequestsToConsider(gomock.Any(), DutyDecisionThresholdMin).Return(
					[]models.ManagementRequest{
						newManagementRequest(101, "toConsiderEmpty", 20, now),
						newManagementRequest(102, "toConsiderDom0", 22, now),
					}, nil,
				)
				cmsdb.EXPECT().GetUnfinishedRequests(gomock.Any(), time.Hour*OkHoursUnfinished).Return(
					[]models.ManagementRequest{
						newManagementRequest(105, "unfinishedEmpty", 3*60+1, now),
						newManagementRequest(106, "unfinishedDom0", 4*60, now),
					}, nil,
				)
				cmsdb.EXPECT().GetResetupRequests(gomock.Any(), time.Hour*OkHoursAtWalle).Return(
					[]models.ManagementRequest{
						newManagementRequest(107, "resetupEmpty", 24*60, now),
						newManagementRequest(108, "resetupDom0", 25*60, now),
					}, nil,
				)
			},
			mockDBM: func(dbmcl *dbmmock.MockClient) {
				empty := make([]dbm.Container, 0)
				dom0 := append(empty, dbm.Container{})
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), "resetupEmpty").Return(empty, nil)
				dbmcl.EXPECT().Dom0Containers(gomock.Any(), "resetupDom0").Return(dom0, nil)
			},
			mockPusher: func(pusher *pushermock.MockPusher, t *testing.T) {
				pusher.EXPECT().Push(gomock.Any(), gomock.Any()).DoAndReturn(func(ctx context.Context, request push.Request) error {
					require.Equal(t, []push.Event{
						{
							Host:        "toConsiderEmpty",
							Service:     "cms_consider_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-101",
						},
						{
							Host:        "toConsiderDom0",
							Service:     "cms_consider_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-102",
						},
						{
							Host:        "unfinishedEmpty",
							Service:     "cms_unfinished_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-105",
						},
						{
							Host:        "unfinishedDom0",
							Service:     "cms_unfinished_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-106",
						},
						{
							Host:        "resetupDom0",
							Service:     "cms_resetup_requests",
							Status:      push.CRIT,
							Description: "https://ui/cms/decision/?q=ext-id-108",
						},
					}, request.Events)
					return nil
				})
			},
		},
	}

	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			mr, ctrl := makeMonrunInteractor(t, tc.now, tc.mockCMSDB, tc.mockDBM, tc.mockPusher)
			defer ctrl.Finish()
			require.NoError(t, mr.SendEvents(ctx))
		})
	}
}
