package steps_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	cms_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	conductormocks "a.yandex-team.ru/cloud/mdb/internal/conductor/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	metadbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
)

var strDrillsConfig = []byte(`
- from: 2021-01-10T14:00:00+03:00
  till: 2021-01-10T15:00:00+03:00
  availability_zones:
    - dc0
  ticket: MDB-1
  wait_before: 2h
- from: 2021-01-20T00:00:00+03:00
  till: 2021-01-21T00:00:00+03:00
  availability_zones:
    - dc1
  ticket: MDB-123
  wait_before: 24h
  wait_after: 24h
- from: 2021-02-20T14:00:00+03:00
  till: 2021-02-20T15:00:00+03:00
  availability_zones:
    - dc2
  ticket: MDB-256
  wait_before: 96h
`)

var drillsConfig = newDrillsConfig()

func newDrillsConfig() steps.DrillsConfig {
	var cfg steps.DrillsConfig
	err := yaml.Unmarshal(strDrillsConfig, &cfg)
	if err != nil {
		fmt.Println(err)
	}
	return cfg
}

func newDate(year int, month time.Month, day, hour, min int) time.Time {
	return time.Date(year, month, day, hour, min, 0, 0, time.FixedZone("MSK", 3*3600))
}

func TestCheckDrills(t *testing.T) {
	testCases := []struct {
		name             string
		today            time.Time
		expectedAction   steps.AfterStepAction
		expectedForHuman string
		dom0             string
	}{
		{
			name:             "not escalating same dc",
			today:            newDate(2021, time.January, 10, 13, 0),
			expectedAction:   steps.AfterStepContinue,
			expectedForHuman: "drills found, but no problems for this event expected",
			dom0:             "somefqdn",
		},
		{
			name:           "escalating before",
			today:          newDate(2021, time.January, 19, 13, 20),
			expectedAction: steps.AfterStepEscalate,
			dom0:           "somefqdn",
			expectedForHuman: "There will be drills in locations dc1 in 10h40m0s, check https://st.yandex-team.ru/MDB-123. " +
				"There are 1 legs in those locations affect this request." +
				"\n* ctnr2 role role, subcid qwe",
		},
		{
			name:           "escalating during 1st day",
			today:          newDate(2021, time.January, 20, 17, 24),
			expectedAction: steps.AfterStepEscalate,
			dom0:           "somefqdn",
			expectedForHuman: "Drills in locations dc1, wait 30h36m0s till 2021-01-22T00:00:00+03:00, check https://st.yandex-team.ru/MDB-123. " +
				"There are 1 legs in those locations affect this request." +
				"\n* ctnr2 role role, subcid qwe",
		},
		{
			name:           "escalating during 2nd day",
			today:          newDate(2021, time.January, 21, 21, 0),
			expectedAction: steps.AfterStepEscalate,
			dom0:           "somefqdn",
			expectedForHuman: "Drills in locations dc1, wait 3h0m0s till 2021-01-22T00:00:00+03:00, check https://st.yandex-team.ru/MDB-123. " +
				"There are 1 legs in those locations affect this request." +
				"\n* ctnr2 role role, subcid qwe",
		},
		{
			name:             "not escalating",
			today:            newDate(2021, time.January, 23, 6, 0),
			expectedAction:   steps.AfterStepContinue,
			dom0:             "somefqdn",
			expectedForHuman: "there are no drills",
		},
		{
			name:             "free date",
			today:            newDate(2021, time.January, 30, 12, 0),
			expectedAction:   steps.AfterStepContinue,
			dom0:             "somefqdn",
			expectedForHuman: "there are no drills",
		},
		{
			name:           "escalating before 4 days",
			today:          newDate(2021, time.February, 17, 13, 7),
			expectedAction: steps.AfterStepEscalate,
			dom0:           "somefqdn",
			expectedForHuman: "There will be drills in locations dc2 in 72h53m0s, check https://st.yandex-team.ru/MDB-256. " +
				"There are 3 legs in those locations affect this request." +
				"\n* ctnr3 role role, subcid qwe" +
				"\n* ctnr4 role role1, role2, subcid qwe" +
				"\n* otherhost, conductor group group1",
		},
		{
			name:           "escalating during",
			today:          newDate(2021, time.February, 20, 14, 15),
			expectedAction: steps.AfterStepEscalate,
			dom0:           "somefqdn",
			expectedForHuman: "Drills in locations dc2, wait 45m0s till 2021-02-20T15:00:00+03:00, check https://st.yandex-team.ru/MDB-256. " +
				"There are 3 legs in those locations affect this request." +
				"\n* ctnr3 role role, subcid qwe" +
				"\n* ctnr4 role role1, role2, subcid qwe" +
				"\n* otherhost, conductor group group1",
		},
		{
			name:             "not escalating after",
			today:            newDate(2021, time.February, 21, 0, 0),
			expectedAction:   steps.AfterStepContinue,
			dom0:             "somefqdn",
			expectedForHuman: "there are no drills",
		},
		{
			name:           "escalating lonely conductor host",
			today:          newDate(2021, time.February, 20, 14, 15),
			expectedAction: steps.AfterStepEscalate,
			dom0:           "strangefqdn",
			expectedForHuman: "Drills in locations dc2, wait 45m0s till 2021-02-20T15:00:00+03:00, check https://st.yandex-team.ru/MDB-256. " +
				"There are 1 legs in those locations affect this request." +
				"\n* lonelyhost has not replicas in conductor group group2",
		},
		{
			name:           "not escalating unknownhost",
			today:          newDate(2021, time.February, 20, 14, 15),
			expectedAction: steps.AfterStepContinue,
			dom0:           "dom0withunknownhost",
			expectedForHuman: "drills found, but no problems for this event expected" +
				"\nAdditional log:" +
				"\n* \"unknownhost\" not found in conductor",
		},
	}

	for _, tc := range testCases {
		ctx := context.Background()
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			meta := metadbmocks.NewMockMetaDB(ctrl)
			meta.EXPECT().Begin(gomock.Any(), gomock.Any()).Times(2).Return(ctx, nil).AnyTimes()
			meta.EXPECT().Rollback(gomock.Any()).Return(nil).AnyTimes()
			meta.EXPECT().Commit(gomock.Any()).Return(nil).AnyTimes()
			meta.EXPECT().GetHostByFQDN(gomock.Any(), "ctnr1").Return(metadb.Host{FQDN: "ctnr1", SubClusterID: "qwe"}, nil).AnyTimes()
			meta.EXPECT().GetHostByFQDN(gomock.Any(), "somehost").Return(metadb.Host{}, metadb.ErrDataNotFound).AnyTimes()
			meta.EXPECT().GetHostByFQDN(gomock.Any(), "lonelyhost").Return(metadb.Host{}, metadb.ErrDataNotFound).AnyTimes()
			meta.EXPECT().GetHostByFQDN(gomock.Any(), "unknownhost").Return(metadb.Host{}, metadb.ErrDataNotFound).AnyTimes()
			meta.EXPECT().GetHostsBySubcid(gomock.Any(), "qwe").Return([]metadb.Host{
				{
					FQDN:         "ctnr1",
					SubClusterID: "qwe",
					Geo:          "dc0",
					Roles:        []string{"role"},
				},
				{
					FQDN:         "ctnr2",
					SubClusterID: "qwe",
					Geo:          "dc1",
					Roles:        []string{"role"},
				},
				{
					FQDN:         "ctnr3",
					SubClusterID: "qwe",
					Geo:          "dc2",
					Roles:        []string{"role"},
				},
				{
					FQDN:         "ctnr4",
					SubClusterID: "qwe",
					Geo:          "dc2",
					Roles:        []string{"role1", "role2"},
				},
			}, nil).AnyTimes()

			dom0d := mocks.NewMockDom0Discovery(ctrl)
			dom0d.EXPECT().Dom0Instances(gomock.Any(), "somefqdn").Return(dom0discovery.DiscoveryResult{
				WellKnown: []cms_models.Instance{{FQDN: "ctnr1"}, {FQDN: "somehost"}},
			}, nil).AnyTimes()
			dom0d.EXPECT().Dom0Instances(gomock.Any(), "strangefqdn").Return(dom0discovery.DiscoveryResult{
				WellKnown: []cms_models.Instance{{FQDN: "lonelyhost"}},
			}, nil).AnyTimes()
			dom0d.EXPECT().Dom0Instances(gomock.Any(), "dom0withunknownhost").Return(dom0discovery.DiscoveryResult{
				WellKnown: []cms_models.Instance{{FQDN: "unknownhost"}},
			}, nil).AnyTimes()
			cncl := conductormocks.NewMockClient(ctrl)
			cncl.EXPECT().HostsInfo(gomock.Any(), []string{"somehost"}).Return([]conductor.HostInfo{
				{
					FQDN:  "somehost",
					DC:    "dc0",
					Group: "group1",
				},
			}, nil).AnyTimes()
			cncl.EXPECT().GroupToHosts(gomock.Any(), "group1", gomock.Any()).Return([]string{"somehost", "otherhost"}, nil).AnyTimes()
			cncl.EXPECT().HostsInfo(gomock.Any(), []string{"somehost", "otherhost"}).Return([]conductor.HostInfo{
				{
					FQDN:  "somehost",
					DC:    "dc0",
					Group: "group1",
				},
				{
					FQDN:  "otherhost",
					DC:    "dc2",
					Group: "group1",
				},
			}, nil).AnyTimes()

			cncl.EXPECT().HostsInfo(gomock.Any(), []string{"lonelyhost"}).Return([]conductor.HostInfo{
				{
					FQDN:  "lonelyhost",
					DC:    "dcx",
					Group: "group2",
				},
			}, nil).AnyTimes()
			cncl.EXPECT().GroupToHosts(gomock.Any(), "group2", gomock.Any()).Return([]string{"lonelyhost"}, nil).AnyTimes()

			cncl.EXPECT().HostsInfo(gomock.Any(), []string{"unknownhost"}).Return(nil, nil).AnyTimes()

			step := steps.NewCheckDrills(drillsConfig, tc.today, meta, dom0d, cncl)
			insCtx := steps.NewEmptyInstructionCtx()

			opsMetaLog := models.NewOpsMetaLog()

			insCtx.SetActualRD(&types.RequestDecisionTuple{
				R: models.ManagementRequest{Fqnds: []string{tc.dom0}},
				D: models.AutomaticDecision{OpsLog: &opsMetaLog},
			})
			res := step.RunStep(ctx, &insCtx)

			require.NoError(t, res.Error)
			require.Equal(t, tc.expectedAction, res.Action)
			require.Equal(t, tc.expectedForHuman, res.ForHuman)
		})
	}
}
