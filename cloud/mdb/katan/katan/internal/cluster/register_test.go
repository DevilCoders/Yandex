package cluster

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
)

func TestGetPlanner(t *testing.T) {
	t.Run("One node cluster plan", func(t *testing.T) {
		cluster := models.Cluster{
			Hosts: map[string]tags.HostTags{"C1": {}},
		}
		composed, err := planner.ComposeCluster(cluster, nil)
		require.NoError(t, err)

		p, err := GetPlanner(cluster)(composed)
		require.NoError(t, err)
		require.Equal(t, [][]string{{"C1"}}, p)
	})
}
