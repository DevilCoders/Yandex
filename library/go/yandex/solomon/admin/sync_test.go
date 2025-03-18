package admin_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/admin"
)

func TestSync(t *testing.T) {
	client := testClient(t)

	syncAndCheck := func(clusters []solomon.Cluster, services []solomon.Service, shards []solomon.Shard, removeGarbage bool) {
		require.NoError(t, admin.Sync(context.Background(), client, clusters, services, shards, &admin.SyncOptions{RemoveGarbage: removeGarbage}))

		actualClusters, err := client.ListClusters(context.Background())
		require.NoError(t, err)

		for _, cluster := range clusters {
			found := false
			for _, realCluster := range actualClusters {
				if cluster.ID == realCluster.ID {
					found = true
				}
			}
			assert.True(t, found, "cluster %#v", cluster)
		}

		if removeGarbage {
			assert.Equal(t, len(actualClusters), len(clusters))
		}

		actualServices, err := client.ListServices(context.Background())
		require.NoError(t, err)

		for _, service := range services {
			found := false
			for _, realService := range actualServices {
				if service.ID == realService.ID {
					found = true
				}
			}

			assert.True(t, found, "service %#v", service)
		}

		if removeGarbage {
			assert.Equal(t, len(actualServices), len(services))
		}

		actualShards, err := client.ListShards(context.Background())
		require.NoError(t, err)

		for _, shard := range shards {
			found := false
			for _, realShard := range actualShards {
				if shard.ClusterID == realShard.ClusterID && shard.ServiceID == realShard.ServiceID {
					found = true
				}
			}

			assert.True(t, found, "shard %#v", shard)
		}

		if removeGarbage {
			assert.Equal(t, len(actualShards), len(shards))
		}
	}

	syncAndCheck(
		[]solomon.Cluster{
			{ID: "test_cluster", Name: "test_cluster"},
		},
		[]solomon.Service{
			{ID: "test_service", Name: "test_service"},
		},
		[]solomon.Shard{
			{ClusterID: "test_cluster", ServiceID: "test_service"},
		},
		false)

	syncAndCheck(
		[]solomon.Cluster{
			{ID: "test_cluster", Name: "test_cluster"},
		},
		[]solomon.Service{
			{ID: "test_service", Name: "test_service"},
			{ID: "test_service2", Name: "test_service"},
		},
		[]solomon.Shard{
			{ClusterID: "test_cluster", ServiceID: "test_service2"},
		},
		true)

	syncAndCheck(nil, nil, nil, false)
	syncAndCheck(nil, nil, nil, true)
}
