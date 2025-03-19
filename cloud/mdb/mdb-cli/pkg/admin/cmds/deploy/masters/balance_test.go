package masters

import (
	"context"
	"math/rand"
	"sort"
	"strings"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	mockdeployapi "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func getTestMinionShipments(
	_ context.Context,
	attrs deployapi.SelectShipmentsAttrs,
	paging deployapi.Paging,
) ([]models.Shipment, deployapi.Paging, error) {

	if strings.HasPrefix(attrs.FQDN.String, "busy-minion-fqdn-") {
		return []models.Shipment{{}}, paging, nil
	}
	return []models.Shipment{}, paging, nil
}

func TestBalancer_Balance(t *testing.T) {
	var testCases = []struct {
		caseName      string
		group         string
		dryRun        bool
		force         bool
		masters       []models.Master
		minions       map[string][]models.Minion
		stat          minionsStat
		error         bool
		shouldBalance bool
	}{
		{
			caseName: "good balanced",
			group:    "fake-group",
			dryRun:   true,
			force:    false,
			masters: []models.Master{
				{FQDN: "master1", Group: "fake-group", IsOpen: true},
			},
			minions: map[string][]models.Minion{
				"master1": {
					{MasterFQDN: "master1", AutoReassign: true},
				},
			},
			stat: minionsStat{
				"master1": {minionCount: 1},
			},
			error: true,
		},
		{
			caseName: "ignore deleted",
			group:    "fake-group",
			dryRun:   true,
			force:    false,
			masters: []models.Master{
				{FQDN: "master1", Group: "fake-group", IsOpen: true},
			},
			minions: map[string][]models.Minion{
				"master1": {
					{MasterFQDN: "master1", AutoReassign: true},
					{MasterFQDN: "master1", Deleted: true, AutoReassign: true},
				},
			},
			stat: minionsStat{
				"master1": {minionCount: 1},
			},
			error: true,
		},
		{
			caseName: "ignore another group",
			group:    "fake-group",
			dryRun:   true,
			force:    false,
			masters: []models.Master{
				{FQDN: "master1", Group: "fake-group", IsOpen: true},
				{FQDN: "master2", Group: "another-group", IsOpen: true},
			},
			minions: map[string][]models.Minion{
				"master1": {
					{MasterFQDN: "master1", AutoReassign: true},
				},
				"master2": {
					{MasterFQDN: "master2", AutoReassign: true},
					{MasterFQDN: "master2", AutoReassign: true},
				},
			},
			stat: minionsStat{
				"master1": {minionCount: 1},
			},
			error: true,
		},
		{
			caseName: "simple balance",
			group:    "fake-group",
			dryRun:   false,
			force:    false,
			masters: []models.Master{
				{FQDN: "master1", Group: "fake-group", IsOpen: true},
				{FQDN: "master2", Group: "fake-group", IsOpen: true},
			},
			minions: map[string][]models.Minion{
				"master1": {
					{MasterFQDN: "master1", AutoReassign: true},
					{MasterFQDN: "master1", AutoReassign: true},
					{MasterFQDN: "master1", AutoReassign: true},
					{MasterFQDN: "master1", AutoReassign: true},
					{MasterFQDN: "master1", AutoReassign: true},
					{MasterFQDN: "master1", AutoReassign: true},
					{MasterFQDN: "master1", AutoReassign: true},
					{MasterFQDN: "master1", AutoReassign: true},
				},
				"master2": {
					{MasterFQDN: "master2", AutoReassign: true},
					{MasterFQDN: "master2", AutoReassign: true},
				},
			},
			stat: minionsStat{
				"master1": {minionCount: 8, minionDebt: 3, fromProbability: 0.375, resultCount: 4},
				"master2": {minionCount: 2, minionDebt: -3, toProbability: 1, resultCount: 6},
			},
			error:         false,
			shouldBalance: true,
		},
		{
			caseName: "all busy minions",
			group:    "fake-group",
			dryRun:   false,
			force:    false,
			masters: []models.Master{
				{FQDN: "master1", Group: "fake-group", IsOpen: true},
				{FQDN: "master2", Group: "fake-group", IsOpen: true},
			},
			minions: map[string][]models.Minion{
				"master1": {
					{MasterFQDN: "master1", AutoReassign: true, FQDN: "busy-minion-fqdn-1"},
					{MasterFQDN: "master1", AutoReassign: true, FQDN: "busy-minion-fqdn-2"},
					{MasterFQDN: "master1", AutoReassign: true, FQDN: "busy-minion-fqdn-3"},
					{MasterFQDN: "master1", AutoReassign: true, FQDN: "busy-minion-fqdn-4"},
				},
				"master2": {},
			},
			stat: minionsStat{
				"master1": {minionCount: 4, minionDebt: 2, fromProbability: 0.5},
				"master2": {minionCount: 0, minionDebt: -2, toProbability: 1, resultCount: 0},
			},
			error:         false,
			shouldBalance: true,
		},
		{
			caseName: "there are busy minions, but vacant ones are enough",
			group:    "fake-group",
			dryRun:   false,
			force:    false,
			masters: []models.Master{
				{FQDN: "master1", Group: "fake-group", IsOpen: true},
				{FQDN: "master2", Group: "fake-group", IsOpen: true},
			},
			minions: map[string][]models.Minion{
				"master1": {
					{MasterFQDN: "master1", AutoReassign: true, FQDN: "busy-minion-fqdn-1"},
					{MasterFQDN: "master1", AutoReassign: true, FQDN: "busy-minion-fqdn-2"},
					{MasterFQDN: "master1", AutoReassign: true, FQDN: "vacant-minion-fqdn-1"},
					{MasterFQDN: "master1", AutoReassign: true, FQDN: "vacant-minion-fqdn-2"},
				},
				"master2": {},
			},
			stat: minionsStat{
				"master1": {minionCount: 4, minionDebt: 2, fromProbability: 0.5},
				"master2": {minionCount: 0, minionDebt: -2, toProbability: 1, resultCount: 2},
			},
			error:         false,
			shouldBalance: true,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.caseName, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			dapi := mockdeployapi.NewMockClient(ctrl)

			dapi.EXPECT().GetMasters(gomock.Any(), gomock.Any()).Return(tc.masters, deployapi.Paging{Size: int64(len(tc.masters) + 1)}, nil)
			var allMinions []models.Minion
			for masterFQDN, minions := range tc.minions {
				allMinions = append(allMinions, minions...)
				dapi.EXPECT().GetMinionsByMaster(gomock.Any(), masterFQDN, gomock.Any()).Return(minions, deployapi.Paging{Size: int64(len(minions) + 1)}, nil).AnyTimes()
			}
			if tc.shouldBalance {
				dapi.EXPECT().GetMinions(gomock.Any(), gomock.Any()).Return(allMinions, deployapi.Paging{Size: int64(len(allMinions) + 1)}, nil)
				dapi.EXPECT().UpsertMinion(gomock.Any(), gomock.Any(), gomock.Any()).Return(models.Minion{}, nil).AnyTimes()
			} else {
				dapi.EXPECT().UpsertMinion(gomock.Any(), gomock.Any(), gomock.Any()).Times(0)
			}
			dapi.EXPECT().GetShipments(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(getTestMinionShipments).AnyTimes()

			b := newBalancer(dapi, &nop.Logger{}, rand.New(rand.NewSource(2)))
			stat, err := b.Balance(context.Background(), tc.group, tc.dryRun, tc.force)
			if tc.error {
				require.Error(t, err)
			} else {
				require.NoError(t, err)
			}
			require.Equal(t, tc.stat, stat)
		})
	}
}
func TestBalancer_probabilities(t *testing.T) {
	var testCases = []struct {
		caseName string
		stats    minionsStat
		newstats minionsStat
		bounds   []masterBounds
		varcoef  float64
	}{
		{
			caseName: "big varcoef",
			varcoef:  0.40,
			stats: minionsStat{
				"master1": {minionCount: 10},
				"master2": {minionCount: 20},
				"master3": {minionCount: 30},
			},
			newstats: minionsStat{
				"master1": {minionCount: 10, minionDebt: -10, toProbability: 1.0},
				"master2": {minionCount: 20},
				"master3": {minionCount: 30, minionDebt: 10, fromProbability: 1. / 3},
			},
			bounds: []masterBounds{{fqdn: "master1", lower: 0, upper: 1.0}},
		},
		{
			caseName: "small varcoef",
			varcoef:  0.2,
			stats: minionsStat{
				"master1": {minionCount: 15},
				"master2": {minionCount: 20},
				"master3": {minionCount: 25},
			},
			newstats: minionsStat{
				"master1": {minionCount: 15, minionDebt: -5, toProbability: 1.0},
				"master2": {minionCount: 20},
				"master3": {minionCount: 25, minionDebt: 5, fromProbability: 1. / 5},
			},
			bounds: []masterBounds{{fqdn: "master1", lower: 0, upper: 1.0}},
		},
		{
			caseName: "zero varcoef",
			varcoef:  0.0,
			stats: minionsStat{
				"master1": {minionCount: 20},
				"master2": {minionCount: 20},
				"master3": {minionCount: 20},
			},
			newstats: minionsStat{
				"master1": {minionCount: 20},
				"master2": {minionCount: 20},
				"master3": {minionCount: 20},
			},
		},
		{
			caseName: "several source masters",
			varcoef:  0.43,
			stats: minionsStat{
				"master1": {minionCount: 9},
				"master2": {minionCount: 21},
				"master3": {minionCount: 9},
			},
			newstats: minionsStat{
				"master1": {minionCount: 9, minionDebt: -4, toProbability: 0.5},
				"master2": {minionCount: 21, minionDebt: 8, fromProbability: 0.38095238095238093},
				"master3": {minionCount: 9, minionDebt: -4, toProbability: 0.5},
			},
			bounds: []masterBounds{{fqdn: "master1", lower: 0, upper: 0.5}, {fqdn: "master3", lower: 0.5, upper: 1.0}},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.caseName, func(t *testing.T) {
			b := newBalancer(nil, &nop.Logger{}, rand.New(rand.NewSource(2)))
			stats, varcoef, err := b.probabilities(tc.stats)
			require.NoError(t, err)
			require.InDelta(t, tc.varcoef, varcoef, 0.01)
			require.Equal(t, tc.newstats, stats)

			bounds := b.bounds(stats)
			sort.Slice(bounds, func(i, j int) bool {
				return bounds[i].fqdn < bounds[j].fqdn
			})
			require.ElementsMatch(t, tc.bounds, bounds)
		})
	}
}
