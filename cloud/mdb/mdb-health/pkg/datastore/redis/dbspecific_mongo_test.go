package redis_test

import (
	"fmt"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	mdbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/mongodb"
	dbspecifictests "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

func createMongoServiceHealthAlive(now time.Time, role string) []types.ServiceHealth {
	service, ok := mongodb.RoleToService()[role]
	if !ok {
		panic("invalid mongodb role")
	}
	return []types.ServiceHealth{
		types.NewServiceHealth(
			service[0],
			now,
			types.ServiceStatusAlive,
			types.ServiceRole(role),
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil,
		),
	}
}

func generateClusterTopologyByRWTests(ctype metadb.ClusterType, roles, shards dbspecific.HostsMap, test dbspecifictests.TestRWMode) []datastore.ClusterTopology {
	if len(roles) != 1 {
		return nil
	}
	res := make([]datastore.ClusterTopology, 0, len(roles))
	for role, allfqdns := range roles {
		hosts := make([]metadb.Host, 0, len(allfqdns))
		for shard, fqdns := range shards {
			for _, fqdn := range fqdns {
				host := metadb.Host{
					FQDN:         fqdn,
					SubClusterID: "subsid-" + shard,
					ShardID:      optional.NewString(shard),
					Roles:        []string{role},
					CreatedAt:    time.Now(),
				}
				hosts = append(hosts, host)
			}
		}
		res = append(res, datastore.ClusterTopology{
			CID: "cluster-env-" + uuid.Must(uuid.NewV4()).String(),
			Rev: 1,
			Cluster: metadb.Cluster{
				Name:        test.Name,
				Type:        ctype,
				Environment: "dev",
				Visible:     true,
				Status:      "RUNNING",
			},
			Hosts: hosts,
		})
	}
	return res
}

func TestDBSpecificMongoSLACheck(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	cid := "cid-test-dbspecific-mongo-sla-check"
	usedEnv := "test"
	masterDC := "vla"
	dcList := []string{"man", "sas", "vla"}
	shardList := []string{"shard1", "shard2", "shard3"}
	hosts := make([]metadb.Host, 0, len(dcList)*(len(shardList)+2))
	shardEnvs := make(map[string]string)

	for _, dc := range dcList {
		hosts = append(hosts,
			metadb.Host{
				FQDN:  fmt.Sprintf("%s-%s.db.yandex.net", dc, "mongocfg"),
				Geo:   dc,
				Roles: []string{mongodb.RoleMongocfg},
			},
			metadb.Host{
				FQDN:  fmt.Sprintf("%s-%s.db.yandex.net", dc, "mongos"),
				Geo:   dc,
				Roles: []string{mongodb.RoleMongos},
			})
		for _, shard := range shardList {
			shardEnvs[shard] = usedEnv
			hosts = append(hosts, metadb.Host{
				FQDN:    fmt.Sprintf("%s-%s.db.yandex.net", dc, shard),
				Geo:     dc,
				Roles:   []string{mongodb.RoleMongod},
				ShardID: optional.NewString(shard),
			})
		}
	}
	topology := datastore.ClusterTopology{
		CID: cid,
		Rev: 1,
		Cluster: metadb.Cluster{
			Name:        "cluster name",
			Type:        metadb.MongodbCluster,
			Environment: usedEnv,
			Visible:     true,
			Status:      "RUNNING",
		},
		Hosts: hosts,
	}

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mp := map[string]metadb.CustomRole{}
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(mp, nil)

	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))

	// set hosts health
	initTS := time.Unix(time.Now().UTC().Unix(), 0)
	for _, host := range hosts {
		services := createMongoServiceHealthAlive(initTS, host.Roles[0])
		mode := types.Mode{
			Timestamp: initTS,
			Read:      true,
			Write:     host.Geo == masterDC,
		}
		require.NoError(t, ds.StoreHostHealth(ctx, types.NewHostHealthWithMode(cid, host.FQDN, services, &mode), hostHealthTimeout))
	}
	require.NoError(t, doProceedCycle(ctx, ds, metadb.PostgresqlCluster), "process update cycle")

	expectCluster := types.ClusterHealth{
		Cid:      cid,
		Env:      usedEnv,
		SLA:      true,
		Status:   types.ClusterStatusAlive,
		StatusTS: initTS,
	}

	allInfo := types.DBRWInfo{
		HostsTotal: 3,
		HostsRead:  3,
		HostsWrite: 1,
		DBTotal:    1,
		DBRead:     1,
		DBWrite:    1,
	}
	twoHostInfo := types.DBRWInfo{
		HostsTotal: 2,
		HostsRead:  2,
		HostsWrite: 1,
		DBTotal:    1,
		DBRead:     1,
		DBWrite:    1,
	}

	fewInfo, err := ds.LoadFewClustersHealth(ctx, metadb.MongodbCluster, "")
	require.NoError(t, err)
	require.Equal(t, datastore.EndCursor, fewInfo.NextCursor)
	require.Equal(t, 1, len(fewInfo.Clusters), "should got our cluster")
	require.WithinDuration(t, initTS, fewInfo.Clusters[0].StatusTS, time.Second, "and have expected sid")
	fewInfo.Clusters[0].StatusTS = initTS
	require.Equal(t, expectCluster, fewInfo.Clusters[0], "and have expected cluster")
	require.Equal(t, 1, len(fewInfo.ClusterInfo))
	clusterInfo, ok := fewInfo.ClusterInfo[cid]
	require.True(t, ok)
	require.Equal(t, types.DBRWInfo{
		HostsTotal: 9,
		HostsRead:  9,
		HostsWrite: 3,
		DBTotal:    3,
		DBRead:     3,
		DBWrite:    3,
	}, clusterInfo)
	require.Equal(t, 3, len(fewInfo.SLAShards))
	for _, shard := range shardList {
		info, ok := fewInfo.SLAShards[shard]
		require.True(t, ok, "shard shoul exist")
		require.Equal(t, allInfo, info, fmt.Sprintf("some issue on shard: %s", shard))
	}
	require.Equal(t, 0, len(fewInfo.NoSLAShards))
	require.Equal(t, 3, len(fewInfo.ShardEnv))
	require.Equal(t, shardEnvs, fewInfo.ShardEnv)

	excludeShard := "shard2"

	// exclude sas host from excludeShard
	upHosts := make([]metadb.Host, 0, len(hosts))
	for _, host := range hosts {
		shard, err := host.ShardID.Get()
		if err == nil && shard == excludeShard && host.Geo == "sas" {
			continue
		}
		upHosts = append(upHosts, host)
	}
	require.Equal(t, len(hosts)-1, len(upHosts))
	topology.Hosts = upHosts
	topology.Rev++
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))
	expectCluster.SLA = false // it loose SLA flag, because excludeShard

	fewInfo, err = ds.LoadFewClustersHealth(ctx, metadb.MongodbCluster, "")
	require.NoError(t, err)
	require.Equal(t, datastore.EndCursor, fewInfo.NextCursor)
	require.Equal(t, 1, len(fewInfo.Clusters), "should got our cluster")
	require.WithinDuration(t, initTS, fewInfo.Clusters[0].StatusTS, time.Second, "and have expected sid")
	fewInfo.Clusters[0].StatusTS = initTS
	require.Equal(t, expectCluster, fewInfo.Clusters[0], "and have expected cluster")
	require.Equal(t, 1, len(fewInfo.ClusterInfo))
	clusterInfo, ok = fewInfo.ClusterInfo[cid]
	require.True(t, ok)
	require.Equal(t, types.DBRWInfo{
		HostsTotal: 8,
		HostsRead:  8,
		HostsWrite: 3,
		DBTotal:    3,
		DBRead:     3,
		DBWrite:    3,
	}, clusterInfo)
	require.Equal(t, 2, len(fewInfo.SLAShards))
	for _, shard := range shardList {
		info, ok := fewInfo.SLAShards[shard]
		if shard == excludeShard {
			require.False(t, ok, "shard shold be in NoSLAShards map")
			info, ok = fewInfo.NoSLAShards[shard]
			require.Equal(t, twoHostInfo, info)
		} else {
			require.Equal(t, allInfo, info, fmt.Sprintf("some issue on shard: %s", shard))
		}
		require.True(t, ok, "shard should exist")
	}
	require.Equal(t, 1, len(fewInfo.NoSLAShards))
	require.Equal(t, 3, len(fewInfo.ShardEnv))
	require.Equal(t, shardEnvs, fewInfo.ShardEnv)

	// exclude all shard excludeShard
	upHosts = make([]metadb.Host, 0, len(hosts))
	for _, host := range hosts {
		shard, err := host.ShardID.Get()
		if err == nil && shard == excludeShard {
			continue
		}
		upHosts = append(upHosts, host)
	}
	require.Equal(t, len(hosts)-3, len(upHosts))
	topology.Hosts = upHosts
	topology.Rev++
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))
	expectCluster.SLA = true // it got SLA flag, because all shards are SLA

	fewInfo, err = ds.LoadFewClustersHealth(ctx, metadb.MongodbCluster, "")
	require.NoError(t, err)
	require.Equal(t, datastore.EndCursor, fewInfo.NextCursor)
	require.Equal(t, 1, len(fewInfo.Clusters), "should got our cluster")
	require.WithinDuration(t, initTS, fewInfo.Clusters[0].StatusTS, time.Second, "and have expected sid")
	fewInfo.Clusters[0].StatusTS = initTS
	require.Equal(t, expectCluster, fewInfo.Clusters[0], "and have expected cluster")
	require.Equal(t, 1, len(fewInfo.ClusterInfo))
	clusterInfo, ok = fewInfo.ClusterInfo[cid]
	require.True(t, ok)
	require.Equal(t, types.DBRWInfo{
		HostsTotal: 6,
		HostsRead:  6,
		HostsWrite: 2,
		DBTotal:    2,
		DBRead:     2,
		DBWrite:    2,
	}, clusterInfo)
	require.Equal(t, 0, len(fewInfo.NoSLAShards))
	require.Equal(t, 2, len(fewInfo.SLAShards))
	for _, shard := range shardList {
		info, ok := fewInfo.SLAShards[shard]
		if shard == excludeShard {
			require.False(t, ok, "shard not exist because is removed shard")
			continue
		}
		require.True(t, ok, "shard should exist")
		require.Equal(t, allInfo, info, fmt.Sprintf("some issue on shard: %s", shard))
	}
	require.Equal(t, 2, len(fewInfo.ShardEnv))
}
func TestDBSpecificMongoHostNeighbours(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	cid := "cid-test-dbspecific-mongo-hostneighbours-check"
	usedEnv := "dev"
	masterDC := "vla"
	dcList := []string{"man", "sas", "vla"}
	shardList := []string{"shard1", "shard2", "shard3"}
	hosts := make([]metadb.Host, 0, len(dcList)*(len(shardList)+2))
	shardEnvs := make(map[string]string)
	reqFQDNS := make([]string, 0, len(shardList))
	excludeShard := "shard2"
	shard2fqdn := make(map[string]string, len(shardList))

	for _, dc := range dcList {
		hosts = append(hosts,
			metadb.Host{
				FQDN:  fmt.Sprintf("%s-%s.db.yandex.net", dc, "mongocfg"),
				Geo:   dc,
				Roles: []string{mongodb.RoleMongocfg},
			},
			metadb.Host{
				FQDN:  fmt.Sprintf("%s-%s.db.yandex.net", dc, "mongos"),
				Geo:   dc,
				Roles: []string{mongodb.RoleMongos},
			})
		for _, shard := range shardList {
			shardEnvs[shard] = usedEnv
			host := metadb.Host{
				FQDN:    fmt.Sprintf("%s-%s.db.yandex.net", dc, shard),
				Geo:     dc,
				Roles:   []string{mongodb.RoleMongod},
				ShardID: optional.NewString(shard),
			}
			hosts = append(hosts, host)
			if dc == "vla" {
				reqFQDNS = append(reqFQDNS, host.FQDN)
				shard2fqdn[shard] = host.FQDN
			}
		}
	}
	topology := datastore.ClusterTopology{
		CID: cid,
		Rev: 1,
		Cluster: metadb.Cluster{
			Name:        "cluster name",
			Type:        metadb.MongodbCluster,
			Environment: usedEnv,
			Visible:     true,
			Status:      "RUNNING",
		},
		Hosts: hosts,
	}

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mp := map[string]metadb.CustomRole{}
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(mp, nil)

	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))

	// set hosts health
	initTS := time.Unix(time.Now().UTC().Unix(), 0)
	for _, host := range hosts {
		services := createMongoServiceHealthAlive(initTS, host.Roles[0])
		mode := types.Mode{
			Timestamp: initTS,
			Read:      true,
			Write:     host.Geo == masterDC,
		}
		require.NoError(t, ds.StoreHostHealth(ctx, types.NewHostHealthWithMode(cid, host.FQDN, services, &mode), hostHealthTimeout))
	}
	require.NoError(t, doProceedCycle(ctx, ds, metadb.PostgresqlCluster), "process update cycle")

	expectCluster := types.ClusterHealth{
		Cid:      cid,
		Env:      usedEnv,
		SLA:      true,
		Status:   types.ClusterStatusAlive,
		StatusTS: initTS,
	}

	expectNeighboursAll := types.HostNeighboursInfo{
		Cid:            cid,
		Env:            usedEnv,
		Roles:          []string{mongodb.RoleMongod},
		HACluster:      true,
		HAShard:        true,
		SameRolesTotal: 2,
		SameRolesAlive: 2,
		SameRolesTS:    initTS,
	}
	expectNeighboursNonHA := types.HostNeighboursInfo{
		Cid:            cid,
		Sid:            excludeShard,
		Env:            usedEnv,
		Roles:          []string{mongodb.RoleMongod},
		HACluster:      false,
		HAShard:        false,
		SameRolesTotal: 1,
		SameRolesAlive: 1,
	}
	hostsNeighboursInfo, err := ds.GetHostNeighboursInfo(ctx, reqFQDNS)
	require.NoError(t, err)
	require.Equal(t, 3, len(hostsNeighboursInfo))
	for _, shard := range shardList {
		hni, ok := hostsNeighboursInfo[shard2fqdn[shard]]
		require.True(t, ok)
		require.WithinDuration(t, expectNeighboursAll.SameRolesTS, hni.SameRolesTS, time.Second, "should have same TS")
		expectNeighboursAll.Sid = shard
		expectNeighboursAll.SameRolesTS = hni.SameRolesTS
		require.Equal(t, expectNeighboursAll, hni, "should got expected neighbours")
	}

	// STEP 2: exclude sas host from excludeShard
	upHosts := make([]metadb.Host, 0, len(hosts))
	for _, host := range hosts {
		shard, err := host.ShardID.Get()
		if err == nil && shard == excludeShard && host.Geo == "sas" {
			continue
		}
		upHosts = append(upHosts, host)
	}
	require.Equal(t, len(hosts)-1, len(upHosts))
	topology.Hosts = upHosts
	topology.Rev++
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))
	expectCluster.SLA = false // it loose SLA flag, because excludeShard

	hostsNeighboursInfo, err = ds.GetHostNeighboursInfo(ctx, reqFQDNS)
	require.NoError(t, err)
	require.Equal(t, 3, len(hostsNeighboursInfo))
	expectNeighboursAll.HACluster = false // cluster not HA because of shard2
	for _, shard := range shardList {
		if shard == excludeShard {
			continue
		}
		hni, ok := hostsNeighboursInfo[shard2fqdn[shard]]
		require.True(t, ok)
		require.WithinDuration(t, expectNeighboursAll.SameRolesTS, hni.SameRolesTS, time.Second, "should have same TS")
		expectNeighboursAll.Sid = shard
		expectNeighboursAll.SameRolesTS = hni.SameRolesTS
		require.Equal(t, expectNeighboursAll, hni, "should got expected neighbours")
	}
	nonHA, ok := hostsNeighboursInfo[shard2fqdn[excludeShard]]
	require.True(t, ok)
	expectNeighboursNonHA.SameRolesTS = nonHA.SameRolesTS
	require.Equal(t, expectNeighboursNonHA, nonHA, "should got expected neighbours for non HA")

	// STEP 3: exclude whole excludeShard
	upHosts = make([]metadb.Host, 0, len(hosts))
	for _, host := range hosts {
		shard, err := host.ShardID.Get()
		if err == nil && shard == excludeShard {
			continue
		}
		upHosts = append(upHosts, host)
	}
	require.Equal(t, len(hosts)-3, len(upHosts))
	topology.Hosts = upHosts
	topology.Rev++
	require.NoError(t, ds.SetClustersTopology(ctx, []datastore.ClusterTopology{topology}, topologyTimeout, mdb))
	expectCluster.SLA = true // it got SLA flag, because all shards are SLA
	hostsNeighboursInfo, err = ds.GetHostNeighboursInfo(ctx, reqFQDNS)
	require.NoError(t, err)
	require.Equal(t, 2, len(hostsNeighboursInfo))
	expectNeighboursAll.HACluster = true // cluster is HA because no shard2 exist
	for _, shard := range shardList {
		if shard == excludeShard {
			continue
		}
		hni, ok := hostsNeighboursInfo[shard2fqdn[shard]]
		require.True(t, ok)
		require.WithinDuration(t, expectNeighboursAll.SameRolesTS, hni.SameRolesTS, time.Second, "should have same TS")
		expectNeighboursAll.Sid = shard
		expectNeighboursAll.SameRolesTS = hni.SameRolesTS
		require.Equal(t, expectNeighboursAll, hni, "should got expected neighbours")
	}
	nonHA, ok = hostsNeighboursInfo[shard2fqdn[excludeShard]]
	require.False(t, ok)
}

func TestDBSpecificMongo(t *testing.T) {

	tests := mongodb.GetTestServices()
	require.NotEqual(t, 0, len(tests))

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	for i, test := range tests {
		t.Run(fmt.Sprintf("%s status %d", test.Expected, i), func(t *testing.T) {
			ctx, ds := initRedis(t)
			defer closeRedis(ctx, ds)
			cid, _, roles, health := dbspecifictests.GenerateOneHostsForRoleStatus(mongodb.RoleToService(), test.Input)
			topology := generateClusterTopologyByHealth(metadb.MongodbCluster, cid, roles, health)
			require.NoError(t, ds.SetClustersTopology(ctx, topology, topologyTimeout, mdb))
			for fqdn, sh := range health {
				hh := types.NewHostHealth(cid, fqdn, sh)
				require.NoError(t, ds.StoreHostHealth(ctx, hh, time.Minute))
			}
			require.NoError(t, doProceedCycle(ctx, ds, metadb.MongodbCluster), "process update cycle")
			clusterHealth, err := ds.GetClusterHealth(ctx, cid)
			require.NoError(t, err)
			require.Equal(t, cid, clusterHealth.Cid)
			require.Equal(t, test.Expected, clusterHealth.Status)
		})
	}
}

func TestDBSpecificRWModesMongo(t *testing.T) {
	tests := mongodb.GetTestRWModes()
	require.NotEqual(t, 0, len(tests.Cases))

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	for _, test := range tests.Cases {
		t.Run(test.Name, func(t *testing.T) {
			ctx, ds := initRedis(t)
			defer closeRedis(ctx, ds)
			topology := generateClusterTopologyByRWTests(metadb.MongodbCluster, tests.Roles, tests.Shards, test)
			require.Equal(t, 1, len(topology))
			cid := topology[0].CID
			require.NoError(t, ds.SetClustersTopology(ctx, topology, topologyTimeout, mdb))
			for fqdn, mode := range test.HealthMode {
				hh := types.NewHostHealthWithMode(cid, fqdn, nil, &mode)
				require.NoError(t, ds.StoreHostHealth(ctx, hh, time.Minute))
			}
			require.NoError(t, doProceedCycle(ctx, ds, metadb.MongodbCluster), "process update cycle")
			fewInfo, err := ds.LoadFewClustersHealth(ctx, metadb.MongodbCluster, "")
			require.NoError(t, err)
			require.Equal(t, datastore.EndCursor, fewInfo.NextCursor)
			mode, ok := fewInfo.ClusterInfo[cid]
			require.True(t, ok)
			require.Equal(t, test.Result, mode)
		})
	}
}
