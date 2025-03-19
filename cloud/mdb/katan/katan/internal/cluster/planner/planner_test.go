package planner_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
)

func C(fqdns ...string) planner.Cluster {
	hosts := make(map[string]planner.Host, len(fqdns))
	for _, f := range fqdns {
		hosts[f] = planner.Host{}
	}
	return planner.Cluster{Hosts: hosts}
}

func TestDefaultRolloutPlanner(t *testing.T) {
	t.Run("For empty set returns empty set", func(t *testing.T) {
		plan, err := planner.Default(planner.Cluster{})
		require.NoError(t, err)
		require.Len(t, plan, 0)
	})

	t.Run("Sort host", func(t *testing.T) {
		plan, err := planner.Default(C("b.db", "a.db", "c.db"))
		require.NoError(t, err)
		require.Equal(t, [][]string{{"a.db"}, {"b.db", "c.db"}}, plan)
	})

	t.Run("Two hosts", func(t *testing.T) {
		plan, _ := planner.Default(C("a.db", "b.db"))
		require.Equal(t, [][]string{{"a.db"}, {"b.db"}}, plan)
	})

	t.Run("Max concurrency is 32", func(t *testing.T) {
		var fqdns []string
		for i := 0; i < 1000; i++ {
			fqdns = append(fqdns, fmt.Sprintf("f-%d", i))
		}
		plan, _ := planner.Default(C(fqdns...))
		require.Len(t, plan[len(plan)-2], 32)
	})
}

func TestNeedHostsHealth(t *testing.T) {
	t.Run("managed 2 node cluster", func(t *testing.T) {
		require.True(t, planner.NeedHostsHealth(models.Cluster{
			ID:    "cid",
			Tags:  tags.ClusterTags{Meta: tags.ClusterMetaTags{Type: metadb.MysqlCluster}},
			Hosts: map[string]tags.HostTags{"h1": {}, "h2": {}},
		}))
	})

	t.Run("managed 1 node cluster", func(t *testing.T) {
		require.False(t, planner.NeedHostsHealth(models.Cluster{
			ID:    "cid",
			Tags:  tags.ClusterTags{Meta: tags.ClusterMetaTags{Type: metadb.MysqlCluster}},
			Hosts: map[string]tags.HostTags{"h1": {}},
		}))
	})

	t.Run("unmanaged cluster", func(t *testing.T) {
		require.False(t, planner.NeedHostsHealth(models.Cluster{
			ID:    "cid",
			Hosts: map[string]tags.HostTags{"h1": {}, "h2": {}},
		}))
	})
}
