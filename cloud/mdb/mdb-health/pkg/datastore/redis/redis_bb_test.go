package redis_test

import (
	"context"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
)

/*
	Generalized redis datastore test, works for both 'fake' redis and real one.
	Which test to run is decided by build tags (unit_test or integration_test)
*/

const (
	secretTimeout       = time.Hour
	hostHealthTimeout   = 15 * time.Second
	clusterHealthTimout = 15 * time.Second
	topologyTimeout     = 24 * time.Hour
)

func generateClusterTopologyByHealth(clusterType metadb.ClusterType, cid string, roles dbspecific.HostsMap, health dbspecific.HealthMap) []datastore.ClusterTopology {
	var metadbHosts []metadb.Host
	for role, hosts := range roles {
		for _, fqdn := range hosts {
			_, ok := health[fqdn]
			if !ok {
				continue
			}
			h := metadb.Host{
				FQDN:         fqdn,
				SubClusterID: "host-subcid-" + uuid.Must(uuid.NewV4()).String(),
				Geo:          "host-geo-" + uuid.Must(uuid.NewV4()).String(),
				Roles:        []string{role},
			}
			metadbHosts = append(metadbHosts, h)
		}
	}
	c := metadb.Cluster{
		Name:        cid,
		Type:        clusterType,
		Environment: "cluster-env-" + uuid.Must(uuid.NewV4()).String(),
		Visible:     true,
		Status:      "RUNNING",
	}
	ct := datastore.ClusterTopology{
		CID:     cid,
		Rev:     1,
		Cluster: c,
		Hosts:   metadbHosts,
	}
	return []datastore.ClusterTopology{ct}
}

func doProceedCycle(ctx context.Context, ds datastore.Backend, ctype metadb.ClusterType) error {
	for cursor := ""; cursor != datastore.EndCursor; {
		fewInfo, err := ds.LoadFewClustersHealth(ctx, ctype, cursor)
		cursor = fewInfo.NextCursor
		if err != nil {
			return err
		}
		err = ds.SaveClustersHealth(ctx, fewInfo.Clusters, clusterHealthTimout)
		if err != nil {
			return err
		}
	}
	return nil
}

func compareServices(t *testing.T, expected, got []types.ServiceHealth) {
	require.Equal(t, len(expected), len(got))
	for _, check := range expected {
		for _, search := range got {
			if check.Name() == search.Name() {
				require.Equal(t, check, search)
				break
			}
		}
	}
}

// Functions below exist only to enable code-completion in the rest of the test.
// Their implementations are defined separately and built depending on build tags.

func initRedis(t *testing.T) (context.Context, datastore.Backend) {
	return initRedisImpl(t, false)
}

func initBadRedis(t *testing.T) (context.Context, datastore.Backend) {
	return initRedisImpl(t, true)
}

func closeRedis(ctx context.Context, ds datastore.Backend) {
	closeRedisImpl(ctx, ds)
}

func fastForwardRedis(ctx context.Context, d time.Duration) {
	fastForwardRedisImpl(ctx, d)
}
