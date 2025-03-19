package mongodb

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

func TestMongoDBEvaluateClusterHealth(t *testing.T) {
	tests := GetTestServices()
	var e extractor
	for i, test := range tests {
		t.Run(fmt.Sprintf("%s status %d", test.Expected, i), func(t *testing.T) {
			cid, fqdns, roles, health := testhelpers.GenerateOneHostsForRoleStatus(RoleToService(), test.Input)
			clusterHealth, err := e.EvaluateClusterHealth(cid, fqdns, roles, health)
			require.NoError(t, err)
			require.Equal(t, cid, clusterHealth.Cid)
			require.Equal(t, test.Expected, clusterHealth.Status)
		})
	}
}

func TestEvaluateClusterHealthWithBrokenRoles(t *testing.T) {
	cases := []struct {
		name   string
		roles  dbspecific.HostsMap
		health dbspecific.HealthMap
	}{
		{
			"cluster without mongod",
			dbspecific.HostsMap{RoleMongocfg: {"m1"}},
			dbspecific.HealthMap{
				"m1": {types.NewAliveHealthForService(RoleMongocfg)},
			},
		},
		{
			"shared cluster with mongos, but without mongocfg",
			dbspecific.HostsMap{RoleMongod: {"m1", "m2"}, RoleMongocfg: {"m3"}},
			dbspecific.HealthMap{
				"m1": {types.NewAliveHealthForService(RoleMongod)},
				"m2": {types.NewAliveHealthForService(RoleMongod)},
				"m3": {types.NewAliveHealthForService(RoleMongocfg)},
			},
		},
		{
			"shared cluster with mongocfg, but without mongos",
			dbspecific.HostsMap{RoleMongod: {"m1", "m2"}, RoleMongos: {"m3"}},
			dbspecific.HealthMap{
				"m1": {types.NewAliveHealthForService(RoleMongod)},
				"m2": {types.NewAliveHealthForService(RoleMongod)},
				"m3": {types.NewAliveHealthForService(RoleMongos)},
			},
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			e := extractor{}
			clusterHealth, err := e.EvaluateClusterHealth(
				"test-cid",
				nil,
				tt.roles,
				tt.health,
			)
			require.Error(t, err)
			require.Equal(t, types.ClusterStatusUnknown, clusterHealth.Status)
		})
	}
}

func TestEvaluateClusterHealthForMultiNodeCluster(t *testing.T) {
	alivePrimary := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongod,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	aliveSecondary := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongod,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeAsync,
			"",
			0,
			nil),
	}
	dead := []types.ServiceHealth{types.NewDeadHealthForService(ServiceMongod)}
	roles := dbspecific.HostsMap{RoleMongod: {"m1", "m2", "m3"}}

	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"m1": alivePrimary,
				"m2": aliveSecondary,
				"m3": aliveSecondary,
			},
			types.ClusterStatusAlive,
		},
		{
			"one replica is dead",
			dbspecific.HealthMap{
				"m1": alivePrimary,
				"m2": aliveSecondary,
				"m3": dead,
			},
			types.ClusterStatusDegraded,
		},
		{
			"master is dead (actually no master)",
			dbspecific.HealthMap{
				"m1": dead,
				"m2": aliveSecondary,
				"m3": aliveSecondary,
			},
			types.ClusterStatusDegraded,
		},
		{
			"there are 2 masters",
			dbspecific.HealthMap{
				"m1": alivePrimary,
				"m2": alivePrimary,
				"m3": aliveSecondary,
			},
			types.ClusterStatusAlive,
		},
		{
			"one node is unknown",
			dbspecific.HealthMap{
				"m1": alivePrimary,
				"m2": aliveSecondary,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"m1": dead,
				"m2": dead,
				"m3": dead,
			},
			types.ClusterStatusDead,
		},
		{
			"all nodes are unknown",
			dbspecific.HealthMap{},
			types.ClusterStatusUnknown,
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			e := extractor{}
			clusterHealth, err := e.EvaluateClusterHealth(
				"test-cid",
				nil,
				roles,
				tt.health,
			)
			require.NoError(t, err)
			require.Equal(t, tt.clusterStatus, clusterHealth.Status)
		})
	}
}

func TestEvaluateClusterHealthForShardedCluster(t *testing.T) {
	aliveMongodPrimary := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongod,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	aliveMongodSecondary := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongod,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeAsync,
			"",
			0,
			nil),
	}
	deadMongod := []types.ServiceHealth{types.NewDeadHealthForService(ServiceMongod)}

	aliveMongos := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongos,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	deadMongos := []types.ServiceHealth{
		types.NewDeadHealthForService(ServiceMongos),
	}

	aliveMongoCFGPrimary := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongocfg,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	aliveMongoCFGSecondary := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongocfg,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	deadMongoCFG := []types.ServiceHealth{
		types.NewDeadHealthForService(ServiceMongocfg),
	}

	roles := dbspecific.HostsMap{
		RoleMongod:   {"d1", "d2", "d3"},
		RoleMongocfg: {"c1", "c2", "c3"},
		RoleMongos:   {"s1", "s2", "s3"},
	}

	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"s1": aliveMongos,
				"s2": aliveMongos,
				"s3": aliveMongos,
				"c1": aliveMongoCFGPrimary,
				"c2": aliveMongoCFGPrimary,
				"c3": aliveMongoCFGPrimary,
			},
			types.ClusterStatusAlive,
		},
		{
			"some unknown",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"s1": aliveMongos,
				"s2": aliveMongos,
				"c1": aliveMongoCFGPrimary,
				"c2": aliveMongoCFGPrimary,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all unknown",
			dbspecific.HealthMap{},
			types.ClusterStatusUnknown,
		},
		{
			"one mongod is dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": deadMongod,
				"s1": aliveMongos,
				"s2": aliveMongos,
				"s3": aliveMongos,
				"c1": aliveMongoCFGPrimary,
				"c2": aliveMongoCFGSecondary,
				"c3": aliveMongoCFGSecondary,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one cfg and one S are dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": deadMongod,
				"s1": aliveMongos,
				"s2": aliveMongos,
				"s3": deadMongos,
				"c1": aliveMongoCFGPrimary,
				"c2": aliveMongoCFGSecondary,
				"c3": deadMongoCFG,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all monogos are dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"s1": deadMongos,
				"s2": deadMongos,
				"s3": deadMongos,
				"c1": aliveMongoCFGPrimary,
				"c2": aliveMongoCFGSecondary,
				"c3": aliveMongoCFGSecondary,
			},
			types.ClusterStatusDead,
		},
		{
			"all cfg are dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"s1": aliveMongos,
				"s2": aliveMongos,
				"s3": aliveMongos,
				"c1": deadMongoCFG,
				"c2": deadMongoCFG,
				"c3": deadMongoCFG,
			},
			types.ClusterStatusDead,
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			e := extractor{}
			clusterHealth, err := e.EvaluateClusterHealth(
				"test-cid",
				nil,
				roles,
				tt.health,
			)
			require.NoError(t, err)
			require.Equal(t, tt.clusterStatus, clusterHealth.Status)
		})
	}
}

func TestEvaluateClusterHealthForShardedClusterWithMonogoInfra(t *testing.T) {
	aliveMongodPrimary := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongod,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	aliveMongodSecondary := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongod,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeAsync,
			"",
			0,
			nil),
	}
	deadMongod := []types.ServiceHealth{types.NewDeadHealthForService(ServiceMongod)}

	aliveMongoCFGPrimaryAliveMongos := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongocfg,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
		types.NewServiceHealth(
			ServiceMongos,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	aliveMongoCFGPrimaryDeadMongos := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongocfg,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
		types.NewDeadHealthForService(ServiceMongos),
	}
	aliveMongoCFGSecondaryAliveMongos := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongocfg,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
		types.NewServiceHealth(
			ServiceMongos,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	aliveMongoCFGSecondaryDeadMongos := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceMongocfg,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
		types.NewDeadHealthForService(ServiceMongos),
	}
	deadMongoCFGAliveMongos := []types.ServiceHealth{
		types.NewDeadHealthForService(ServiceMongocfg),
		types.NewServiceHealth(
			ServiceMongos,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	deadMongoCFGDeadMongos := []types.ServiceHealth{
		types.NewDeadHealthForService(ServiceMongocfg),
		types.NewDeadHealthForService(ServiceMongos),
	}

	roles := dbspecific.HostsMap{
		RoleMongod:     {"d1", "d2", "d3"},
		RoleMongoinfra: {"i1", "i2", "i3"},
	}

	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": aliveMongoCFGSecondaryAliveMongos,
				"i3": aliveMongoCFGSecondaryAliveMongos,
			},
			types.ClusterStatusAlive,
		},
		{
			"some unknown",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": aliveMongoCFGSecondaryAliveMongos,
			},
			types.ClusterStatusDegraded,
		},
		{
			"some unknown 2",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": aliveMongoCFGSecondaryAliveMongos,
			},
			types.ClusterStatusDegraded,
		},
		{
			"some unknown 3",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": aliveMongoCFGSecondaryAliveMongos,
				"i3": aliveMongoCFGSecondaryAliveMongos,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all unknown",
			dbspecific.HealthMap{},
			types.ClusterStatusUnknown,
		},
		{
			"one mongod is dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": deadMongod,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": aliveMongoCFGSecondaryAliveMongos,
				"i3": aliveMongoCFGSecondaryAliveMongos,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one cfg and one S are dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": deadMongod,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": deadMongoCFGAliveMongos,
				"i3": aliveMongoCFGSecondaryDeadMongos,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all monogos are dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"i1": aliveMongoCFGPrimaryDeadMongos,
				"i2": aliveMongoCFGSecondaryDeadMongos,
				"i3": aliveMongoCFGSecondaryDeadMongos,
			},
			types.ClusterStatusDead,
		},
		{
			"all cfg are dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"i1": deadMongoCFGAliveMongos,
				"i2": deadMongoCFGAliveMongos,
				"i3": deadMongoCFGAliveMongos,
			},
			types.ClusterStatusDead,
		},
		{
			"one S dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": aliveMongoCFGSecondaryDeadMongos,
				"i3": aliveMongoCFGSecondaryAliveMongos,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one cfg dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": deadMongoCFGAliveMongos,
				"i3": aliveMongoCFGSecondaryAliveMongos,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one Infra dead",
			dbspecific.HealthMap{
				"d1": aliveMongodPrimary,
				"d2": aliveMongodSecondary,
				"d3": aliveMongodSecondary,
				"i1": aliveMongoCFGPrimaryAliveMongos,
				"i2": deadMongoCFGDeadMongos,
				"i3": aliveMongoCFGSecondaryAliveMongos,
			},
			types.ClusterStatusDegraded,
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			e := extractor{}
			clusterHealth, err := e.EvaluateClusterHealth(
				"test-cid",
				nil,
				roles,
				tt.health,
			)
			require.NoError(t, err)
			require.Equal(t, tt.clusterStatus, clusterHealth.Status)
		})
	}
}

func TestHasSLAGuaranties(t *testing.T) {
	dataNoShardHostMan := metadb.Host{
		FQDN:  "man-data-no-shard.db.yandex.net",
		Geo:   "man",
		Roles: []string{RoleMongod},
	}
	dataNoShardHostIva := metadb.Host{
		FQDN:  "iva-data-no-shard.db.yandex.net",
		Geo:   "iva",
		Roles: []string{RoleMongod},
	}

	dataNoShardHostMyt := metadb.Host{
		FQDN:  "myt-data-no-shard.db.yandex.net",
		Geo:   "myt",
		Roles: []string{RoleMongod},
	}

	dataNoShardHostSas := metadb.Host{
		FQDN:  "sas-data-no-shard.db.yandex.net",
		Geo:   "sas",
		Roles: []string{RoleMongod},
	}
	dataNoShardHostVla := metadb.Host{
		FQDN:  "vla-data-no-shard.db.yandex.net",
		Geo:   "vla",
		Roles: []string{RoleMongod},
	}

	cfgHostMan := metadb.Host{
		FQDN:  "man-mongocfg.db.yandex.net",
		Geo:   "man",
		Roles: []string{RoleMongocfg},
	}
	cfgHostIva := metadb.Host{
		FQDN:  "iva-mongocfg.db.yandex.net",
		Geo:   "iva",
		Roles: []string{RoleMongocfg},
	}

	cfgHostMyt := metadb.Host{
		FQDN:  "myt-mongocfg.db.yandex.net",
		Geo:   "myt",
		Roles: []string{RoleMongocfg},
	}

	cfgHostSas := metadb.Host{
		FQDN:  "sas-mongocfg.db.yandex.net",
		Geo:   "sas",
		Roles: []string{RoleMongocfg},
	}
	cfgHostVla := metadb.Host{
		FQDN:  "vla-mongocfg.db.yandex.net",
		Geo:   "vla",
		Roles: []string{RoleMongocfg},
	}

	sHostMyt := metadb.Host{
		FQDN:  "myt-mongos.db.yandex.net",
		Geo:   "myt",
		Roles: []string{RoleMongos},
	}

	sHostSas := metadb.Host{
		FQDN:  "sas-mongos.db.yandex.net",
		Geo:   "sas",
		Roles: []string{RoleMongos},
	}
	sHostSas2 := metadb.Host{
		FQDN:  "sas-mongos2.db.yandex.net",
		Geo:   "sas",
		Roles: []string{RoleMongos},
	}
	sHostVla := metadb.Host{
		FQDN:  "vla-mongos.db.yandex.net",
		Geo:   "vla",
		Roles: []string{RoleMongos},
	}

	infraHostMan := metadb.Host{
		FQDN:  "man-mongoinfra.db.yandex.net",
		Geo:   "man",
		Roles: []string{RoleMongoinfra},
	}
	infraHostSas := metadb.Host{
		FQDN:  "sas-mongoinfra.db.yandex.net",
		Geo:   "sas",
		Roles: []string{RoleMongoinfra},
	}
	infraHostVla := metadb.Host{
		FQDN:  "vla-mongoinfra.db.yandex.net",
		Geo:   "vla",
		Roles: []string{RoleMongoinfra},
	}

	dataShard1HostMan := metadb.Host{
		ShardID: optional.NewString("shard1"),
		FQDN:    "man-data-shard1.db.yandex.net",
		Geo:     "man",
		Roles:   []string{RoleMongod},
	}
	dataShard1HostIva := metadb.Host{
		ShardID: optional.NewString("shard1"),
		FQDN:    "iva-data-shard1.db.yandex.net",
		Geo:     "iva",
		Roles:   []string{RoleMongod},
	}

	dataShard1HostMyt := metadb.Host{
		ShardID: optional.NewString("shard1"),
		FQDN:    "myt-data-shard1.db.yandex.net",
		Geo:     "myt",
		Roles:   []string{RoleMongod},
	}

	dataShard1HostSas := metadb.Host{
		ShardID: optional.NewString("shard1"),
		FQDN:    "sas-data-shard1.db.yandex.net",
		Geo:     "sas",
		Roles:   []string{RoleMongod},
	}
	dataShard1HostVla := metadb.Host{
		ShardID: optional.NewString("shard1"),
		FQDN:    "vla-data-shard1.db.yandex.net",
		Geo:     "vla",
		Roles:   []string{RoleMongod},
	}

	dataShard2HostMan := metadb.Host{
		ShardID: optional.NewString("shard2"),
		FQDN:    "man-data-shard2.db.yandex.net",
		Geo:     "man",
		Roles:   []string{RoleMongod},
	}
	dataShard2HostIva := metadb.Host{
		ShardID: optional.NewString("shard2"),
		FQDN:    "iva-data-shard2.db.yandex.net",
		Geo:     "iva",
		Roles:   []string{RoleMongod},
	}

	dataShard2HostMyt := metadb.Host{
		ShardID: optional.NewString("shard2"),
		FQDN:    "myt-data-shard2.db.yandex.net",
		Geo:     "myt",
		Roles:   []string{RoleMongod},
	}

	dataShard2HostSas := metadb.Host{
		ShardID: optional.NewString("shard2"),
		FQDN:    "sas-data-shard2.db.yandex.net",
		Geo:     "sas",
		Roles:   []string{RoleMongod},
	}
	dataShard2HostVla := metadb.Host{
		ShardID: optional.NewString("shard2"),
		FQDN:    "vla-data-shard2.db.yandex.net",
		Geo:     "vla",
		Roles:   []string{RoleMongod},
	}

	dataShard3HostMyt := metadb.Host{
		ShardID: optional.NewString("shard3"),
		FQDN:    "vla-data-shard3.db.yandex.net",
		Geo:     "ru-central1-a",
		Roles:   []string{RoleMongod},
	}
	dataShard3HostSas := metadb.Host{
		ShardID: optional.NewString("shard3"),
		FQDN:    "sas-data-shard3.db.yandex.net",
		Geo:     "ru-central1-b",
		Roles:   []string{RoleMongod},
	}
	dataShard3HostVla := metadb.Host{
		ShardID: optional.NewString("shard3"),
		FQDN:    "myt-data-shard3.db.yandex.net",
		Geo:     "ru-central1-c",
		Roles:   []string{RoleMongod},
	}

	cases := []struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		{
			"no shard sla cluster",
			[]metadb.Host{
				dataNoShardHostMyt,
				dataNoShardHostSas,
				dataNoShardHostVla,
			},
			true,
			nil,
		},
		{
			"no shard sla cluster extra",
			[]metadb.Host{
				dataNoShardHostMan,
				dataNoShardHostMyt,
				dataNoShardHostIva,
				dataNoShardHostSas,
				dataNoShardHostVla,
			},
			true,
			nil,
		},
		{
			"no shard no sla cluster",
			[]metadb.Host{
				dataNoShardHostIva,
			},
			false,
			nil,
		},
		{
			"no shard no sla cluster extra",
			[]metadb.Host{
				dataNoShardHostMan,
				dataNoShardHostIva,
			},
			false,
			nil,
		},
		{
			"sharded cloud without mongocfg and mongos",
			[]metadb.Host{
				dataShard3HostMyt,
				dataShard3HostSas,
				dataShard3HostVla,
			},
			true,
			map[string]bool{"shard3": true},
		},
		{
			"sharded without mongocfg and mongos",
			[]metadb.Host{
				dataShard1HostMyt,
				dataShard1HostSas,
				dataShard1HostVla,
			},
			true,
			map[string]bool{"shard1": true},
		},
		{
			"sharded with mongoinfra only",
			[]metadb.Host{
				dataShard1HostMyt,
				dataShard1HostSas,
				dataShard1HostVla,
				dataShard2HostMyt,
				dataShard2HostSas,
				dataShard2HostVla,
				infraHostMan,
				infraHostSas,
				infraHostVla,
			},
			true,
			map[string]bool{"shard1": true, "shard2": true},
		},
		{
			"mongoinfra not under SLA",
			[]metadb.Host{
				dataShard1HostMyt,
				dataShard1HostSas,
				dataShard1HostVla,
				dataShard2HostMyt,
				dataShard2HostSas,
				dataShard2HostVla,
				infraHostMan,
				infraHostSas,
			},
			false,
			map[string]bool{"shard1": false, "shard2": false},
		},
		{
			"sharded with mongocfg only",
			[]metadb.Host{
				dataShard1HostMan,
				dataShard1HostIva,
				dataShard1HostVla,
				dataShard2HostMan,
				dataShard2HostIva,
				dataShard2HostVla,
				cfgHostMan,
				cfgHostIva,
				cfgHostVla,
			},
			false,
			map[string]bool{"shard1": false, "shard2": false},
		},
		{
			"two mongos under SLA",
			[]metadb.Host{
				dataShard1HostMyt,
				dataShard1HostSas,
				dataShard1HostVla,
				dataShard2HostMyt,
				dataShard2HostSas,
				dataShard2HostVla,
				cfgHostMyt,
				cfgHostSas,
				cfgHostVla,
				sHostSas,
				sHostVla,
			},
			true,
			map[string]bool{"shard1": true, "shard2": true},
		},
		{
			"mongos not under SLA",
			[]metadb.Host{
				dataShard1HostMyt,
				dataShard1HostSas,
				dataShard1HostVla,
				dataShard2HostMyt,
				dataShard2HostSas,
				dataShard2HostVla,
				cfgHostMyt,
				cfgHostSas,
				cfgHostVla,
				sHostSas,
				sHostSas2,
			},
			false,
			map[string]bool{"shard1": false, "shard2": false},
		},
		{
			"mongocfg not under SLA",
			[]metadb.Host{
				dataShard1HostMyt,
				dataShard1HostSas,
				dataShard1HostVla,
				dataShard2HostMyt,
				dataShard2HostSas,
				dataShard2HostVla,
				cfgHostSas,
				cfgHostVla,
				sHostMyt,
				sHostSas,
				sHostVla,
			},
			false,
			map[string]bool{"shard1": false, "shard2": false},
		},
		{
			"sharded partial by sla",
			[]metadb.Host{
				dataShard1HostMyt,
				dataShard1HostVla,
				dataShard2HostMyt,
				dataShard2HostSas,
				dataShard2HostVla,
				cfgHostMyt,
				cfgHostSas,
				cfgHostVla,
				sHostMyt,
				sHostSas,
				sHostVla,
			},
			false,
			map[string]bool{"shard1": false, "shard2": true},
		},
		{
			"sharded",
			[]metadb.Host{
				dataShard1HostMyt,
				dataShard1HostSas,
				dataShard1HostVla,
				dataShard2HostMyt,
				dataShard2HostSas,
				dataShard2HostVla,
				cfgHostMyt,
				cfgHostSas,
				cfgHostVla,
				sHostMyt,
				sHostSas,
				sHostVla,
			},
			true,
			map[string]bool{"shard1": true, "shard2": true},
		},
		{
			"one shard by sla",
			[]metadb.Host{
				dataShard1HostSas,
				dataShard1HostVla,
				dataShard2HostMyt,
				dataShard2HostSas,
				dataShard2HostVla,
				cfgHostMyt,
				cfgHostSas,
				cfgHostVla,
				sHostMyt,
				sHostSas,
				sHostVla,
			},
			false,
			map[string]bool{"shard1": false, "shard2": true},
		},
		{
			"two shard by sla",
			[]metadb.Host{
				dataShard1HostMan,
				dataShard1HostIva,
				dataShard1HostVla,
				dataShard2HostMan,
				dataShard2HostIva,
				dataShard2HostVla,
				cfgHostMan,
				cfgHostSas,
				cfgHostVla,
				sHostMyt,
				sHostSas,
				sHostVla,
			},
			true,
			map[string]bool{"shard1": true, "shard2": true},
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			e := extractor{}
			topology := datastore.ClusterTopology{
				CID: "cid-" + tt.name,
				Rev: 1,
				Cluster: metadb.Cluster{
					Name:        "cluster name: " + tt.name,
					Type:        metadb.MongodbCluster,
					Environment: "qa",
					Visible:     true,
					Status:      "RUNNING",
				},
				Hosts: tt.hosts,
			}
			slaCluster, slaShards := e.GetSLAGuaranties(topology)
			require.Equal(t, tt.slaCluster, slaCluster)
			require.Equal(t, tt.slaShards, slaShards)
		})
	}
}

func TestMongoDBEvaluateDBReadWriteInfo(t *testing.T) {
	var e extractor
	tests := GetTestRWModes()
	for _, test := range tests.Cases {
		t.Run(test.Name, func(t *testing.T) {
			info, err := e.EvaluateDBInfo("test-cid", "RUNNING", tests.Roles, tests.Shards, test.HealthMode, nil)
			require.NoError(t, err)
			require.Equal(t, test.Result, info)
		})
	}
}
