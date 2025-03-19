package redis

import (
	"context"
	"testing"
	"time"

	"github.com/alicebob/miniredis/v2"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/health"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	expiration  = time.Second
	cid         = "cid1"
	cidNotFound = "not_found"
)

type ctxKey string

var (
	ctxKeyMiniRedis = ctxKey("miniredis")
)

func initRedis(t *testing.T) (context.Context, datastore.Backend) {
	ctx := context.Background()
	logger, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, logger)

	mr, err := miniredis.Run()
	require.NoError(t, err)
	require.NotNil(t, mr)
	ctx = context.WithValue(ctx, ctxKeyMiniRedis, mr)

	cfg := Config{
		Addrs:      []string{mr.Addr()},
		Expiration: expiration,
	}

	ds := New(logger, cfg)
	require.NotNil(t, ds)

	return ctx, ds
}

func closeRedis(ctx context.Context, ds datastore.Backend) {
	_ = ds.Close()

	mr := ctx.Value(ctxKeyMiniRedis).(*miniredis.Miniredis)
	mr.Close()
}

func fastForwardRedis(ctx context.Context, d time.Duration) {
	mr := ctx.Value(ctxKeyMiniRedis).(*miniredis.Miniredis)
	mr.FastForward(d)
}

func TestClusterHealthStoreAndLoad(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	clusterHealth := models.ClusterHealth{
		Cid:    cid,
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Hbase: models.ServiceHbase{
				BasicHealthService: models.BasicHealthService{Health: health.Alive},
				Regions:            123,
				Requests:           124,
				AverageLoad:        0.21,
			},
			service.Hdfs: models.ServiceHdfs{
				BasicHealthService:      models.BasicHealthService{Health: health.Degraded},
				PercentRemaining:        0.33,
				Used:                    123,
				Free:                    33,
				TotalBlocks:             4312,
				MissingBlocks:           1,
				MissingBlocksReplicaOne: 4,
			},
			service.Hive: models.ServiceHive{
				BasicHealthService: models.BasicHealthService{Health: health.Degraded},
				QueriesSucceeded:   123,
				QueriesExecuting:   5,
				QueriesFailed:      10,
				SessionsOpen:       1011,
				SessionsActive:     3,
			},
		},
	}

	err := ds.StoreClusterHealth(ctx, cid, clusterHealth)
	require.NoError(t, err)

	got, err := ds.LoadClusterHealth(ctx, clusterHealth.Cid)
	require.NoError(t, err)
	require.Equal(t, clusterHealth, got)

	_, err = ds.LoadClusterHealth(ctx, cidNotFound)
	require.Equal(t, datastore.ErrNotFound, err)
}

func TestClusterHealthTimeout(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	clusterHealth := models.ClusterHealth{Cid: cid}

	err := ds.StoreClusterHealth(ctx, clusterHealth.Cid, clusterHealth)
	require.NoError(t, err)

	fastForwardRedis(ctx, 2*expiration)

	_, err = ds.LoadClusterHealth(ctx, clusterHealth.Cid)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, datastore.ErrNotFound))
}

func TestHostHealthStoreAndLoad(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	hostsHealth := map[string]models.HostHealth{
		"host1": {
			Fqdn:   "host1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hbase: models.ServiceHbaseNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					Requests:           123,
					HeapSizeMb:         567,
					MaxHeapSizeMb:      1024,
				},
			},
		},
		"host2": {
			Fqdn:   "host2",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hbase: models.ServiceHbaseNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					Requests:           312,
					HeapSizeMb:         567,
					MaxHeapSizeMb:      1024,
				},
			},
		},
	}

	err := ds.StoreHostsHealth(ctx, cid, hostsHealth)
	require.NoError(t, err)

	got, err := ds.LoadHostsHealth(ctx, cid, []string{"host1", "host2"})
	require.NoError(t, err)
	require.Equal(t, hostsHealth, got)

	got, err = ds.LoadHostsHealth(ctx, cid, []string{"host1", "host2", "unknown_host"})
	want := map[string]models.HostHealth{
		"host1":        hostsHealth["host1"],
		"host2":        hostsHealth["host2"],
		"unknown_host": models.NewHostUnknownHealth(),
	}
	require.NoError(t, err)
	require.Equal(t, want, got)

	got, err = ds.LoadHostsHealth(ctx, cid, []string{"host1"})
	want = map[string]models.HostHealth{
		"host1": hostsHealth["host1"],
	}
	require.NoError(t, err)
	require.Equal(t, want, got)

	got, err = ds.LoadHostsHealth(ctx, cid, []string{})
	want = map[string]models.HostHealth{}
	require.NoError(t, err)
	require.Equal(t, want, got)

	got, err = ds.LoadHostsHealth(ctx, cidNotFound, []string{"host1", "host2"})
	want = map[string]models.HostHealth{
		"host1": models.NewHostUnknownHealth(),
		"host2": models.NewHostUnknownHealth(),
	}
	require.NoError(t, err)
	require.Equal(t, want, got)
}

func TestHostHealthTimeout(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	hostsHealth := map[string]models.HostHealth{
		"host1": {
			Fqdn: "host1",
		},
	}

	err := ds.StoreHostsHealth(ctx, cid, hostsHealth)
	require.NoError(t, err)

	fastForwardRedis(ctx, 2*expiration)

	got, err := ds.LoadHostsHealth(ctx, cid, []string{"host1", "host2"})
	want := map[string]models.HostHealth{
		"host1": models.NewHostUnknownHealth(),
		"host2": models.NewHostUnknownHealth(),
	}
	require.NoError(t, err)
	require.Equal(t, want, got)
}

func TestTopologyStoreAndLoad(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	topology := models.ClusterTopology{
		Cid:      cid,
		FolderID: "folder1",
		Services: []service.Service{service.Hdfs, service.Yarn, service.Mapreduce, service.Hbase, service.Zookeeper},
		Subclusters: []models.SubclusterTopology{
			{
				Subcid:   "main",
				Role:     role.Main,
				Services: []service.Service{service.Hdfs, service.Yarn, service.Mapreduce, service.Hbase, service.Zookeeper},
				Hosts:    []string{"hdp-m0.internal"},
			},
			{
				Subcid:   "data",
				Role:     role.Data,
				Services: []service.Service{service.Hdfs, service.Yarn, service.Hbase},
				Hosts:    []string{"hdp-d0.internal", "hdp-d1.internal"},
			},
			{
				Subcid:   "data2",
				Role:     role.Compute,
				Services: []service.Service{service.Mapreduce, service.Yarn},
				Hosts:    []string{"hdp-d2.internal"},
			},
		},
	}

	err := ds.StoreClusterTopology(ctx, cid, topology)
	require.NoError(t, err)

	got, err := ds.GetCachedClusterTopology(ctx, cid)
	require.NoError(t, err)
	require.Equal(t, topology, got)

	_, err = ds.GetCachedClusterTopology(ctx, cidNotFound)
	require.Equal(t, datastore.ErrNotFound, err)
}

func TestTopologyTimeout(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	topology := models.ClusterTopology{FolderID: "folder1"}

	err := ds.StoreClusterTopology(ctx, cid, topology)
	require.NoError(t, err)

	fastForwardRedis(ctx, time.Minute*50)
	got, err := ds.GetCachedClusterTopology(ctx, cid)
	require.NoError(t, err)
	require.Equal(t, topology, got)

	// Checking that timeout moved
	fastForwardRedis(ctx, time.Minute*50)
	got, err = ds.GetCachedClusterTopology(ctx, cid)
	require.NoError(t, err)
	require.Equal(t, topology, got)

	// Checking that timeout expired
	fastForwardRedis(ctx, time.Minute*65)
	got, err = ds.GetCachedClusterTopology(ctx, cid)
	require.Error(t, err)
	require.Equal(t, models.ClusterTopology{}, got)
}
