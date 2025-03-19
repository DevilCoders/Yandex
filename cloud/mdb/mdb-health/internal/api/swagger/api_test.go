package swagger

import (
	"context"
	"net/http"
	"sort"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	readymocks "a.yandex-team.ru/cloud/mdb/internal/ready/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/health"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/unhealthy"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types/testhelpers"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestHostHealthCheckReplicaTypeStoreAndLoad(t *testing.T) {
	for serviceType := 0; serviceType < 4; serviceType++ {
		serviceCount := 3
		metricsCount := 3
		hh1 := testhelpers.NewHostHealthSpec(serviceCount, metricsCount, serviceType)
		hhm1 := hostHealthToModel(hh1)
		hh1back, _ := hostHealthFromModel(hhm1)
		require.Equal(t, hh1, hh1back)

		hhm1back := hostHealthToModel(hh1back)
		require.Equal(t, hhm1, hhm1back)
	}
}

func TestUnhealthyAggregatedInfoLoad(t *testing.T) {
	uuai := unhealthy.UAInfo{
		StatusRecs: make(map[unhealthy.StatusKey]*unhealthy.UARecord),
		RWRecs:     make(map[unhealthy.RWKey]*unhealthy.UARWRecord),
	}
	uuai.StatusRecs[unhealthy.StatusKey{Status: string(types.ClusterStatusAlive)}] =
		&unhealthy.UARecord{Count: 2}
	uuai.StatusRecs[unhealthy.StatusKey{Status: string(types.ClusterStatusDead)}] =
		&unhealthy.UARecord{Count: 1, Examples: []string{"cid3"}}
	uuai.RWRecs[unhealthy.RWKey{AggType: types.AggClusters, Readable: true, Writeable: true}] =
		&unhealthy.UARWRecord{Count: 2, NoReadCount: 0, NoWriteCount: 0}
	uuai.RWRecs[unhealthy.RWKey{AggType: types.AggClusters, Readable: false, Writeable: false}] =
		&unhealthy.UARWRecord{Count: 1, NoReadCount: 1, NoWriteCount: 1, Examples: []string{"cid3"}}
	uuai.RWRecs[unhealthy.RWKey{AggType: types.AggClusters, Readable: false, Writeable: false, SLA: true}] =
		&unhealthy.UARWRecord{Count: 1, NoReadCount: 1, NoWriteCount: 1, Examples: []string{"cid3"}}

	muai := unhealthyAggregatedInfoToModel(uuai)
	sort.SliceStable(muai.NoSLA.ByHealth, func(i, j int) bool {
		return muai.NoSLA.ByHealth[i].Status < muai.NoSLA.ByHealth[j].Status
	})
	require.NotNil(t, muai)
	require.Equal(t, models.ClusterStatus(types.ClusterStatusAlive), muai.NoSLA.ByHealth[0].Status)
	require.Equal(t, int64(2), muai.NoSLA.ByHealth[0].Count)
}

func TestRequestID(t *testing.T) {
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	ctrl := gomock.NewController(t)
	readymock := readymocks.NewMockChecker(ctrl)
	api := New(nil, logger, readymock)

	readymock.EXPECT().IsReady(gomock.Any()).DoAndReturn(func(ctx context.Context) error {
		rid, ok := requestid.CheckedFromContext(ctx)
		require.True(t, ok)
		require.NotEmpty(t, rid)
		return nil
	})
	_ = api.GetPingHandler(health.PingParams{HTTPRequest: &http.Request{}})

	expEmptyRid := ""
	readymock.EXPECT().IsReady(gomock.Any()).DoAndReturn(func(ctx context.Context) error {
		rid, ok := requestid.CheckedFromContext(ctx)
		require.True(t, ok)
		require.Empty(t, rid)
		return nil
	})
	_ = api.GetPingHandler(health.PingParams{XRequestID: &expEmptyRid, HTTPRequest: &http.Request{}})

	expValuableRid := requestid.New()
	readymock.EXPECT().IsReady(gomock.Any()).DoAndReturn(func(ctx context.Context) error {
		rid, ok := requestid.CheckedFromContext(ctx)
		require.True(t, ok)
		require.Equal(t, expValuableRid, rid)
		return nil
	})
	_ = api.GetPingHandler(health.PingParams{XRequestID: &expValuableRid, HTTPRequest: &http.Request{}})
}
