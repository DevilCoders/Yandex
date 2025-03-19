package provider

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

func TestClickHouse_ShardGroupProcess(t *testing.T) {
	shards := []clusters.Shard{
		{
			SubClusterID: "subcid_ch",
			ShardID:      "ch_shard1",
			Name:         "shard1",
		},
		{
			SubClusterID: "subcid_ch",
			ShardID:      "ch_shard2",
			Name:         "shard2",
		},
	}

	t.Run("ValidateAndSort", func(t *testing.T) {
		shardNames, err := processClusterShards(shards, []string{"shard2", "shard1"})
		require.NoError(t, err)
		require.Equal(t, []string{"shard1", "shard2"}, shardNames)
	})

	t.Run("Deduplicate", func(t *testing.T) {
		shardNames, err := processClusterShards(shards, []string{"shard2", "shard1", "shard1"})
		require.NoError(t, err)
		require.Equal(t, []string{"shard1", "shard2"}, shardNames)
	})

	t.Run("FailEmpty", func(t *testing.T) {
		shardNames, err := processClusterShards(shards, []string{})
		require.Error(t, err)
		require.True(t, semerr.IsInvalidInput(err))
		require.Equal(t, []string(nil), shardNames)
	})

	t.Run("FailNotFound", func(t *testing.T) {
		shardNames, err := processClusterShards(shards, []string{"shard1", "shard3", "shard2"})
		require.Error(t, err)
		require.True(t, semerr.IsNotFound(err))
		require.Equal(t, []string(nil), shardNames)
	})
}
