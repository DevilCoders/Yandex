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
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
)

func createPGServiceHealthAlive(now time.Time, role types.ServiceRole, replicaType types.ServiceReplicaType) []types.ServiceHealth {
	return []types.ServiceHealth{
		types.NewServiceHealth(
			"pg_replication",
			now,
			types.ServiceStatusAlive,
			role,
			replicaType,
			"",
			0,
			nil,
		),
		types.NewServiceHealth(
			"pgbouncer",
			now,
			types.ServiceStatusAlive,
			types.ServiceRoleUnknown,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil,
		),
	}
}

func TestClusterHealthSetTTLS(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	// Use Postgres as example, cause it simple
	// and 'per-cluster' ClusterHealth calculation logic tested in dbspecific modules.
	//
	// Here we ensure that:
	// - ClusterHealth store with TTL (becomes Unknown when expire)
	// - ClusterHealth becomes Unknown when hosts-health metrics expire

	cid := "test-cid"
	masterFQDN := "pg-man"
	replicaAFQDN := "pg-vla"
	replicaBFQDN := "pg-sas"

	clusterHosts := []metadb.Host{
		{
			FQDN:         masterFQDN,
			SubClusterID: "postgre-subcid",
			Geo:          "man",
			Roles:        []string{"postgresql_cluster"},
		},
		{
			FQDN:         replicaAFQDN,
			SubClusterID: "postgre-subcid",
			Geo:          "vla",
			Roles:        []string{"postgresql_cluster"},
		},
		{
			FQDN:         replicaBFQDN,
			SubClusterID: "postgre-subcid",
			Geo:          "sas",
			Roles:        []string{"postgresql_cluster"},
		},
	}
	topology := datastore.ClusterTopology{
		CID: cid,
		Rev: 42,
		Cluster: metadb.Cluster{
			Name:        "pgCluster",
			Type:        metadb.PostgresqlCluster,
			Environment: "qa",
			Visible:     true,
			Status:      "RUNNING",
		},
		Hosts: clusterHosts,
	}

	now := time.Now()
	hostsServices := map[string][]types.ServiceHealth{
		masterFQDN:   createPGServiceHealthAlive(now, types.ServiceRoleMaster, types.ServiceReplicaTypeUnknown),
		replicaAFQDN: createPGServiceHealthAlive(now, types.ServiceRoleReplica, types.ServiceReplicaTypeQuorum),
		replicaBFQDN: createPGServiceHealthAlive(now, types.ServiceRoleReplica, types.ServiceReplicaTypeQuorum),
	}
	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))
	for fqdn, services := range hostsServices {
		require.NoError(t, ds.StoreHostHealth(ctx, types.NewHostHealth(cid, fqdn, services), hostHealthTimeout))
	}
	require.NoError(t, doProceedCycle(ctx, ds, metadb.PostgresqlCluster), "process update cycle")

	// sanity check - verify that we create healthy cluster
	clusterHealth, err := ds.GetClusterHealth(ctx, cid)
	require.NoError(t, err)
	require.Equal(t, types.ClusterStatusAlive, clusterHealth.Status, "test initialization failed. We create unhealthy cluster.")

	// health neighbours request
	expectNeighbours := types.HostNeighboursInfo{
		Cid:            cid,
		Env:            "qa",
		Roles:          []string{"postgresql_cluster"},
		HACluster:      true,
		HAShard:        false,
		SameRolesTotal: 2,
		SameRolesAlive: 2,
	}
	hni, err := ds.GetHostNeighboursInfo(ctx, []string{replicaAFQDN})
	require.NoError(t, err)
	require.Equal(t, 1, len(hni))
	hn, ok := hni[replicaAFQDN]
	require.True(t, ok)
	require.Equal(t, now.Unix(), hn.SameRolesTS.Unix())
	expectNeighbours.SameRolesTS = hn.SameRolesTS
	require.Equal(t, expectNeighbours, hn)

	// use as greatest timeout + gap
	sleepTime := clusterHealthTimout + hostHealthTimeout
	fastForwardRedis(ctx, sleepTime)

	clusterHealth, err = ds.GetClusterHealth(ctx, cid)
	require.NoError(t, err)
	require.Equal(
		t,
		types.ClusterStatusUnknown,
		clusterHealth.Status,
		"after expire cluster health should be unknown",
	)

	uncontrolHost := "fqdn-any.db.yandex.net"
	hni, err = ds.GetHostNeighboursInfo(ctx, []string{replicaAFQDN, uncontrolHost})
	require.NoError(t, err)
	require.Equal(t, 1, len(hni))
	hn, ok = hni[replicaAFQDN]
	require.True(t, ok)
	expectNeighbours.SameRolesAlive = 0
	expectNeighbours.SameRolesTS = time.Time{}
	require.Equal(t, expectNeighbours, hn, "after expire host neighbours should be not alive")

	require.NoError(t, doProceedCycle(ctx, ds, metadb.PostgresqlCluster), "process update cycle")

	clusterHealth, err = ds.GetClusterHealth(ctx, cid)
	require.NoError(t, err)
	require.Equal(
		t,
		types.ClusterStatusUnknown,
		clusterHealth.Status,
		"after hosts health expire cluster health should be unknown",
	)
}

func TestClusterStatusIsUsedCorrectly(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	fqdn := "fqdn-man.db.yandex.net"
	cid := "test-cluster-status-is-used"
	usedEnv := "qa"
	clusterHosts := []metadb.Host{
		{
			FQDN:         fqdn,
			SubClusterID: "postgre-subcid",
			Geo:          "man",
			Roles:        []string{"postgresql_cluster"},
		},
	}

	// prepare initial topology
	topology := datastore.ClusterTopology{
		CID: cid,
		Rev: 1,
		Cluster: metadb.Cluster{
			Name:        "pgCluster",
			Type:        metadb.PostgresqlCluster,
			Environment: usedEnv,
			Visible:     true,
			Status:      "CREATING",
		},
		Hosts: clusterHosts,
	}
	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)
	// set creating topology
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))

	// set hosts health
	initTS := time.Unix(time.Now().UTC().Unix(), 0)
	hostsServices := map[string][]types.ServiceHealth{
		fqdn: createPGServiceHealthAlive(initTS, types.ServiceRoleMaster, types.ServiceReplicaTypeUnknown),
	}
	for fqdn, services := range hostsServices {
		require.NoError(t, ds.StoreHostHealth(ctx, types.NewHostHealth(cid, fqdn, services), hostHealthTimeout))
	}
	require.NoError(t, doProceedCycle(ctx, ds, metadb.PostgresqlCluster), "process update cycle")

	fewInfo, err := ds.LoadFewClustersHealth(ctx, metadb.PostgresqlCluster, "")
	require.NoError(t, err)
	require.Equal(t, 0, len(fewInfo.Clusters), "cluster is just creating, and should not exist in this list")

	// update topology
	topology.Cluster.Status = "RUNNING"
	topology.Rev++
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))

	expectCluster := types.ClusterHealth{
		Cid:      cid,
		Env:      usedEnv,
		SLA:      false,
		Status:   types.ClusterStatusAlive,
		StatusTS: initTS,
	}
	fewInfo, err = ds.LoadFewClustersHealth(ctx, metadb.PostgresqlCluster, "")
	require.NoError(t, err)
	require.Equal(t, 1, len(fewInfo.Clusters), "running cluster should present")
	require.WithinDuration(t, initTS, fewInfo.Clusters[0].StatusTS, time.Second, "and have expected sid")
	fewInfo.Clusters[0].StatusTS = initTS
	require.Equal(t, expectCluster, fewInfo.Clusters[0], "and have expected cluster")

	// update topology
	topology.Cluster.Status = "MODIFYING"
	topology.Rev++
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))

	fewInfo, err = ds.LoadFewClustersHealth(ctx, metadb.PostgresqlCluster, "")
	require.NoError(t, err)
	require.Equal(t, 1, len(fewInfo.Clusters), "modifying cluster should be still visible")
	require.WithinDuration(t, initTS, fewInfo.Clusters[0].StatusTS, time.Second, "and have expected sid")
	fewInfo.Clusters[0].StatusTS = initTS
	require.Equal(t, expectCluster, fewInfo.Clusters[0], "and have expected cluster")

	// update topology
	topology.Cluster.Status = "MODIFY-ERROR"
	topology.Rev++
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))

	fewInfo, err = ds.LoadFewClustersHealth(ctx, metadb.PostgresqlCluster, "")
	require.NoError(t, err)
	require.Equal(t, 1, len(fewInfo.Clusters), "modify-error cluster also should be still visible")
	require.WithinDuration(t, initTS, fewInfo.Clusters[0].StatusTS, time.Second, "and have expected sid")
	fewInfo.Clusters[0].StatusTS = initTS
	require.Equal(t, expectCluster, fewInfo.Clusters[0], "and have expected cluster")

	// update topology
	topology.Cluster.Status = "DELETING"
	topology.Rev++
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))

	fewInfo, err = ds.LoadFewClustersHealth(ctx, metadb.PostgresqlCluster, "")
	require.NoError(t, err)
	require.Equal(t, 0, len(fewInfo.Clusters), "deleting cluster should not calculated")
}

func TestGeoIsLoadedInLoadFewClustersHealth(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	cid := "test-geo-is-used"
	usedEnv := "qa"
	usedGeo := "vla"
	fqdn := fmt.Sprintf("fqdn-%s.db.yandex.net", usedGeo)
	clusterHosts := []metadb.Host{
		{
			FQDN:         fqdn,
			SubClusterID: "postgre-subcid",
			Geo:          usedGeo,
			Roles:        []string{"postgresql_cluster"},
		},
	}

	// prepare initial topology
	topology := datastore.ClusterTopology{
		CID: cid,
		Rev: 1,
		Cluster: metadb.Cluster{
			Name:        "pgCluster",
			Type:        metadb.PostgresqlCluster,
			Environment: usedEnv,
			Visible:     true,
			Status:      "RUNNING",
		},
		Hosts: clusterHosts,
	}
	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)
	// set creating topology
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))

	// set hosts health
	initTS := time.Unix(time.Now().UTC().Unix(), 0)
	hostsServices := map[string][]types.ServiceHealth{
		fqdn: createPGServiceHealthAlive(initTS, types.ServiceRoleMaster, types.ServiceReplicaTypeUnknown),
	}
	for fqdn, services := range hostsServices {
		require.NoError(t, ds.StoreHostHealth(ctx, types.NewHostHealth(cid, fqdn, services), hostHealthTimeout))
	}
	require.NoError(t, doProceedCycle(ctx, ds, metadb.PostgresqlCluster), "process update cycle")

	expectCluster := types.ClusterHealth{
		Cid:      cid,
		Env:      usedEnv,
		SLA:      false,
		Status:   types.ClusterStatusAlive,
		StatusTS: initTS,
	}
	expectHostInfo := datastore.HostInfo{
		Geo:    usedGeo,
		Status: types.HostStatusAlive,
		Env:    usedEnv,
		TS:     initTS,
	}
	fewInfo, err := ds.LoadFewClustersHealth(ctx, metadb.PostgresqlCluster, "")
	require.NoError(t, err)
	require.Equal(t, 1, len(fewInfo.Clusters), "running cluster should present")
	require.WithinDuration(t, initTS, fewInfo.Clusters[0].StatusTS, time.Second, "and have expected timestamp")
	fewInfo.Clusters[0].StatusTS = initTS
	require.Equal(t, expectCluster, fewInfo.Clusters[0], "and have expected cluster")
	require.Equal(t, 1, len(fewInfo.HostInfo))
	hostInfo, hasHostInfo := fewInfo.HostInfo[fqdn]
	require.True(t, hasHostInfo)
	require.WithinDuration(t, initTS, hostInfo.TS, time.Second, "and have expected timestamp")
	expectHostInfo.TS = hostInfo.TS
	require.Equal(t, expectHostInfo, hostInfo)

	// status should be unknown after hostHealthTimeout
	expectCluster.Status = types.ClusterStatusUnknown
	expectCluster.StatusTS = time.Time{}
	expectHostInfo.Status = types.HostStatusUnknown
	expectHostInfo.TS = time.Time{}
	fastForwardRedis(ctx, hostHealthTimeout)
	fewInfo, err = ds.LoadFewClustersHealth(ctx, metadb.PostgresqlCluster, "")
	require.NoError(t, err)
	require.Equal(t, 1, len(fewInfo.Clusters), "running cluster should present")
	require.Equal(t, expectCluster, fewInfo.Clusters[0], "and have expected cluster")
	//require.Equal(t, types.ClusterStatusUnknown, fewInfo.Clusters[0].Status, "cluster status should be unknown")
	require.Equal(t, 1, len(fewInfo.HostInfo))
	hostInfo, hasHostInfo = fewInfo.HostInfo[fqdn]
	require.True(t, hasHostInfo)
	require.Equal(t, expectHostInfo, hostInfo)
}
