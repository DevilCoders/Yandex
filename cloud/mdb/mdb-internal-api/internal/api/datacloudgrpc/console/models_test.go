package console

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
)

func TestDatacloudConsole_GenerateClusterNameHints(t *testing.T) {
	existingClusters := []consolemodels.Cluster{
		{Type: clusters.TypeClickHouse, Name: "clickhouse1"},
		{Type: clusters.TypeElasticSearch, Name: "elasticsearch_cluster1"},
		{Type: clusters.TypeElasticSearch, Name: "elasticsearch_cluster2"},
	}

	t.Run("01. No clusters", func(t *testing.T) {
		expected := "clickhouse1"
		actual, err := GenerateClusterNameHint(clusters.TypeClickHouse, []consolemodels.Cluster{})
		require.NoError(t, err)
		require.Equal(t, expected, actual)
	})

	t.Run("02. Initial cluster", func(t *testing.T) {
		expected := "kafka1"
		actual, err := GenerateClusterNameHint(clusters.TypeKafka, existingClusters)
		require.NoError(t, err)
		require.Equal(t, expected, actual)
	})

	t.Run("03. User-defined template not counts", func(t *testing.T) {
		expected := "elasticsearch1"
		actual, err := GenerateClusterNameHint(clusters.TypeElasticSearch, existingClusters)
		require.NoError(t, err)
		require.Equal(t, expected, actual)
	})

	t.Run("04. Continue iterating", func(t *testing.T) {
		expected := "clickhouse2"
		actual, err := GenerateClusterNameHint(clusters.TypeClickHouse, existingClusters)
		require.NoError(t, err)
		require.Equal(t, expected, actual)
	})

	t.Run("05. Fail on unknown cluster type", func(t *testing.T) {
		_, err := GenerateClusterNameHint(clusters.TypeUnknown, existingClusters)
		require.Error(t, err)
	})
}
