package redis_test

import (
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	mdbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types/testhelpers"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
)

func requireEqualModes(t *testing.T, expected, actual *types.Mode) {
	if expected == nil || actual == nil {
		require.Equal(t, expected, actual)
		return
	}
	require.Equal(t, expected.Read, actual.Read)
	require.Equal(t, expected.Write, actual.Write)
	require.Equal(t, expected.Timestamp.Unix(), actual.Timestamp.Unix())
}

func requireEqualHostHealth(t *testing.T, expected, actual types.HostHealth) {
	require.Equal(t, expected.ClusterID(), actual.ClusterID())
	require.Equal(t, expected.FQDN(), actual.FQDN())
	requireEqualModes(t, expected.Mode(), actual.Mode())
	require.ElementsMatch(t, expected.Services(), actual.Services())
	if expected.System() == nil {
		require.Nil(t, actual.System())
	} else {
		require.NotNil(t, actual.System())
		if expected.System().CPU == nil {
			require.Nil(t, actual.System().CPU)
		} else {
			require.NotNil(t, expected.System().CPU)
			require.Equal(t, *expected.System().CPU, *actual.System().CPU)
		}

		if expected.System().Memory == nil {
			require.Nil(t, actual.System().Memory)
		} else {
			require.NotNil(t, actual.System().Memory)
			require.Equal(t, *expected.System().Memory, *actual.System().Memory)
		}

		if expected.System().Disk == nil {
			require.Nil(t, actual.System().Disk)
		} else {
			require.NotNil(t, actual.System().Disk)
			require.Equal(t, *expected.System().Disk, *actual.System().Disk)
		}
	}
}

func requireEqualHostHealthArray(t *testing.T, expected, actual []types.HostHealth) {
	require.Len(t, actual, len(expected))
	for _, expectedHH := range expected {
		for _, actualHH := range actual {
			if actualHH.FQDN() == expectedHH.FQDN() {
				requireEqualHostHealth(t, expectedHH, actualHH)
				break
			}
		}
	}
}

func generateClusterTopology(hh types.HostHealth) []datastore.ClusterTopology {
	c := metadb.Cluster{
		Name:        "cluster-name-" + uuid.Must(uuid.NewV4()).String(),
		Type:        metadb.ClusterType("cluster-type-" + uuid.Must(uuid.NewV4()).String()),
		Environment: "cluster-env-" + uuid.Must(uuid.NewV4()).String(),
		Visible:     true,
	}
	h := metadb.Host{
		FQDN:         hh.FQDN(),
		SubClusterID: "host-subcid-" + uuid.Must(uuid.NewV4()).String(),
		Geo:          "host-geo-" + uuid.Must(uuid.NewV4()).String(),
		Roles:        []string{"host-role-" + uuid.Must(uuid.NewV4()).String()},
	}
	ct := datastore.ClusterTopology{
		CID:     hh.ClusterID(),
		Rev:     1,
		Cluster: c,
		Hosts:   []metadb.Host{h},
	}
	return []datastore.ClusterTopology{ct}
}

func TestUnavailableStorage(t *testing.T) {
	ctx, ds := initBadRedis(t)
	defer closeRedis(ctx, ds)

	cid := uuid.Must(uuid.NewV4()).String()
	secret := uuid.Must(uuid.NewV4()).Bytes()

	err := ds.StoreClusterSecret(ctx, cid, secret, secretTimeout)
	require.Error(t, err)
	require.True(t, semerr.IsUnavailable(err))

	_, err = ds.LoadClusterSecret(ctx, cid)
	require.Error(t, err)
	require.True(t, semerr.IsUnavailable(err))

	hh := testhelpers.NewHostHealth(3, 3)
	err = ds.StoreHostHealth(ctx, hh, hostHealthTimeout)
	require.Error(t, err)
	require.True(t, semerr.IsUnavailable(err))

	loadedHostHealths, err := ds.LoadHostsHealth(ctx, []string{hh.FQDN()})
	require.Error(t, err)
	require.True(t, semerr.IsUnavailable(err))
	require.Len(t, loadedHostHealths, 0)
}

func TestStoreHostsHealth(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	hh1 := testhelpers.NewHostHealth(3, 3)
	hh2 := testhelpers.NewHostHealth(3, 3)

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	require.NoError(t, ds.SetClustersTopology(ctx, generateClusterTopology(hh1), topologyTimeout, mdb))
	require.NoError(t, ds.SetClustersTopology(ctx, generateClusterTopology(hh2), topologyTimeout, mdb))

	require.NoError(t, ds.StoreHostsHealth(ctx, []healthstore.HostHealthToStore{
		{
			Health: hh1,
			TTL:    hostHealthTimeout,
		},
		{
			Health: hh2,
			TTL:    hostHealthTimeout,
		},
	}))

	loadedHostsHealth, err := ds.LoadHostsHealth(ctx, []string{hh1.FQDN(), hh2.FQDN()})
	require.NoError(t, err)
	require.Len(t, loadedHostsHealth, 2)
	requireEqualHostHealth(t, hh1, loadedHostsHealth[0])
	requireEqualHostHealth(t, hh2, loadedHostsHealth[1])
}

func TestHostHealthStoreAndLoad(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	hh1 := types.NewHostHealthWithSystemAndMode("cid", "host.db.yandex.net", nil, testhelpers.NewSystemMetrics(), &types.Mode{
		Timestamp: time.Now(),
		Read:      true,
		Write:     true,
	})

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	require.NoError(t, ds.SetClustersTopology(ctx, generateClusterTopology(hh1), topologyTimeout, mdb))
	require.NoError(t, ds.StoreHostHealth(ctx, hh1, hostHealthTimeout))

	loadedHostsHealth, err := ds.LoadHostsHealth(ctx, []string{hh1.FQDN()})
	require.NoError(t, err)
	loadedHostHealth := loadedHostsHealth[0]
	requireEqualHostHealth(t, hh1, loadedHostHealth)

	hh2 := testhelpers.NewHostHealthWithClusterID(hh1.ClusterID(), 3, 3)
	require.NoError(t, ds.SetClustersTopology(ctx, generateClusterTopology(hh2), topologyTimeout, mdb))
	require.NoError(t, ds.StoreHostHealth(ctx, hh2, hostHealthTimeout))

	loadedHostsHealth, err = ds.LoadHostsHealth(ctx, []string{hh2.FQDN()})
	require.NoError(t, err)
	loadedHostHealth = loadedHostsHealth[0]
	requireEqualHostHealth(t, hh2, loadedHostHealth)

	loadedHostHealths, err := ds.LoadHostsHealth(ctx, []string{hh1.FQDN(), hh2.FQDN()})
	require.NoError(t, err)
	require.NotNil(t, loadedHostHealths)
	requireEqualHostHealthArray(t, []types.HostHealth{hh1, hh2}, loadedHostHealths)

	// Try loading unknown host
	unknownFQDN := uuid.Must(uuid.NewV4()).String()
	loadedHostsHealth, err = ds.LoadHostsHealth(ctx, []string{unknownFQDN})
	require.NoError(t, err)
	loadedHostHealth = loadedHostsHealth[0]
	require.Empty(t, loadedHostHealth.ClusterID())
	require.Equal(t, unknownFQDN, loadedHostHealth.FQDN())
	require.Empty(t, loadedHostHealth.Services())
	require.Equal(t, types.NewUnknownHostHealth(unknownFQDN), loadedHostHealth)

	// Update host health with both new and old services
	servicesOverlap := []types.ServiceHealth{testhelpers.NewMasterServiceHealth(3)}
	for _, sh := range hh1.Services() {
		servicesOverlap = append(
			servicesOverlap,
			types.NewServiceHealth(
				sh.Name(),
				sh.Timestamp().Add(time.Second),
				sh.Status(),
				sh.Role(),
				sh.ReplicaType(),
				sh.ReplicaUpstream(),
				sh.ReplicaLag(),
				sh.Metrics(),
			),
		)
	}
	servicesOverlap = append(servicesOverlap, testhelpers.NewMasterServiceHealth(3))
	hhServiceOverlap := testhelpers.NewHostHealthWithOtherServices(hh1, servicesOverlap)

	require.NoError(t, ds.StoreHostHealth(ctx, hhServiceOverlap, hostHealthTimeout))

	loadedHostHealths, err = ds.LoadHostsHealth(ctx, []string{hhServiceOverlap.FQDN()})
	require.NoError(t, err)
	require.NotNil(t, loadedHostHealths)
	requireEqualHostHealthArray(t, []types.HostHealth{hhServiceOverlap}, loadedHostHealths)

	// Update host health with empty system metric value
	hh3 := testhelpers.NewHostHealthWithOtherSystem(hh2, &types.SystemMetrics{
		CPU:    testhelpers.NewCPUMetric(),
		Memory: testhelpers.NewMemoryMetric(),
		Disk:   nil,
	})
	require.NoError(t, ds.StoreHostHealth(ctx, hh3, hostHealthTimeout))

	loadedHostsHealth, err = ds.LoadHostsHealth(ctx, []string{hh3.FQDN()})
	require.NoError(t, err)
	loadedHostHealth = loadedHostsHealth[0]
	require.Equal(t, hh3.FQDN(), loadedHostHealth.FQDN())
	require.Equal(t, hh3.ClusterID(), loadedHostHealth.ClusterID())
	requireEqualModes(t, hh3.Mode(), loadedHostHealth.Mode())
	require.ElementsMatch(t, hh3.Services(), loadedHostHealth.Services())
	require.Equal(t, hh3.System().CPU, loadedHostHealth.System().CPU)
	require.Equal(t, hh3.System().Memory, loadedHostHealth.System().Memory)
	require.Equal(t, hh2.System().Disk, loadedHostHealth.System().Disk)

	// Update host health with empty system metrics
	hhEmptySystem := testhelpers.NewHostHealthWithOtherSystem(hh3, nil)
	require.NoError(t, ds.StoreHostHealth(ctx, hhEmptySystem, hostHealthTimeout))

	loadedHostsHealth, err = ds.LoadHostsHealth(ctx, []string{hhEmptySystem.FQDN()})
	require.NoError(t, err)
	loadedHostHealth = loadedHostsHealth[0]
	require.Equal(t, hhEmptySystem.FQDN(), loadedHostHealth.FQDN())
	require.Equal(t, hhEmptySystem.ClusterID(), loadedHostHealth.ClusterID())
	requireEqualModes(t, hhEmptySystem.Mode(), loadedHostHealth.Mode())
	require.ElementsMatch(t, hhEmptySystem.Services(), loadedHostHealth.Services())
	require.Equal(t, hh3.System().CPU, loadedHostHealth.System().CPU)
	require.Equal(t, hh3.System().Memory, loadedHostHealth.System().Memory)
	require.Equal(t, hh2.System().Disk, loadedHostHealth.System().Disk)
}

func TestHostHealthCheckReplicaTypeStoreAndLoad(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	hasAsync := false
	hasSync := false
	hasQuorum := false
	hasUnexpected := false
	for serviceType := 1; serviceType < 5; serviceType++ {
		hh1 := testhelpers.NewHostHealthSpec(3, 3, serviceType)
		require.NoError(t, ds.SetClustersTopology(ctx, generateClusterTopology(hh1), topologyTimeout, mdb))
		require.NoError(t, ds.StoreHostHealth(ctx, hh1, hostHealthTimeout))

		loadedHostHealth, err := ds.LoadHostsHealth(ctx, []string{hh1.FQDN()})
		require.NoError(t, err)
		requireEqualHostHealth(t, hh1, loadedHostHealth[0])

		for _, sh := range hh1.Services() {
			if sh.Role() == types.ServiceRoleMaster {
				require.Equal(t, sh.ReplicaType(), types.ServiceReplicaTypeUnknown)
			} else if sh.Role() == types.ServiceRoleReplica {
				require.NotEqual(t, sh.ReplicaType(), types.ServiceReplicaTypeUnknown)
				switch sh.ReplicaType() {
				case types.ServiceReplicaTypeAsync:
					hasAsync = true
				case types.ServiceReplicaTypeSync:
					hasSync = true
				case types.ServiceReplicaTypeQuorum:
					hasQuorum = true
				default:
					hasUnexpected = true
				}
			}
		}
	}
	require.True(t, hasAsync)
	require.True(t, hasSync)
	require.True(t, hasQuorum)
	require.False(t, hasUnexpected) // for check
}

func TestHostHealthCheckReplicaUpstreamAndLagStoreAndLoad(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	hh1 := testhelpers.NewHostHealthWithOtherServices(testhelpers.NewHostHealth(1, 0), []types.ServiceHealth{
		testhelpers.NewMasterServiceHealth(0),
		testhelpers.NewQuorumReplicaHealthWithUpstreamAndLag(0, "foobar.db.yandex.net", 42),
	})
	require.NoError(t, ds.SetClustersTopology(ctx, generateClusterTopology(hh1), topologyTimeout, mdb))
	require.NoError(t, ds.StoreHostHealth(ctx, hh1, hostHealthTimeout))

	loadedHostHealth, err := ds.LoadHostsHealth(ctx, []string{hh1.FQDN()})
	require.NoError(t, err)
	requireEqualHostHealth(t, hh1, loadedHostHealth[0])
}

func TestHostHealthTimeout(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	hh := testhelpers.NewHostHealth(3, 3)

	ctrl := gomock.NewController(t)
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	mdb.EXPECT().GetClusterCustomRolesAtRev(
		gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(map[string]metadb.CustomRole{}, nil)

	require.NoError(t, ds.SetClustersTopology(ctx, generateClusterTopology(hh), topologyTimeout, mdb))
	require.NoError(t, ds.StoreHostHealth(ctx, hh, time.Second))

	loadedHostHealth, err := ds.LoadHostsHealth(ctx, []string{hh.FQDN()})
	require.NoError(t, err)
	requireEqualHostHealth(t, hh, loadedHostHealth[0])

	fastForwardRedis(ctx, time.Second)

	loadedHostHealth2, err := ds.LoadHostsHealth(ctx, []string{hh.FQDN()})
	require.NoError(t, err)
	// should be no alive services, because of timeout
	requireEqualHostHealth(t, testhelpers.NewHostHealthWithOtherServicesAndSystem(hh, []types.ServiceHealth{}, nil), loadedHostHealth2[0])
}
