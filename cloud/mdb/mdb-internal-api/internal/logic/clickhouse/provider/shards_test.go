package provider

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestClickHouse_ShardNameGenerator(t *testing.T) {
	t.Run("AddToEmptyList", func(t *testing.T) {
		shardNames, err := generateNewShardNames([]string{}, 2)
		require.NoError(t, err)
		require.Equal(t, []string{"s1", "s2"}, shardNames)
	})

	t.Run("AddToNotEmptyList", func(t *testing.T) {
		shardNames, err := generateNewShardNames([]string{"s1", "s2"}, 2)
		require.NoError(t, err)
		require.Equal(t, []string{"s3", "s4"}, shardNames)
	})
}
