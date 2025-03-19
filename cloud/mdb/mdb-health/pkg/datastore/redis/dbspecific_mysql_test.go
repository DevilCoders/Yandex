package redis_test

import (
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	mdbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/mysql"
	dbspecifictests "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

func TestDBSpecificMySQL(t *testing.T) {

	tests := mysql.GetTestServices()
	require.NotEqual(t, 0, len(tests))
	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	for i, test := range tests {
		t.Run(fmt.Sprintf("%s status %d", test.Expected, i), func(t *testing.T) {
			ctx, ds := initRedis(t)
			defer closeRedis(ctx, ds)
			cid, _, roles, health := dbspecifictests.GenerateOneHostsForRoleStatus(mysql.RoleToService(), test.Input)
			topology := generateClusterTopologyByHealth(metadb.MysqlCluster, cid, roles, health)
			require.NoError(t, ds.SetClustersTopology(ctx, topology, topologyTimeout, mdb))
			for fqdn, sh := range health {
				hh := types.NewHostHealth(cid, fqdn, sh)
				require.NoError(t, ds.StoreHostHealth(ctx, hh, time.Minute))
			}
			require.NoError(t, doProceedCycle(ctx, ds, metadb.MysqlCluster), "process update cycle")
			clusterHealth, err := ds.GetClusterHealth(ctx, cid)
			require.NoError(t, err)
			require.Equal(t, cid, clusterHealth.Cid)
			require.Equal(t, test.Expected, clusterHealth.Status)
		})
	}
}
