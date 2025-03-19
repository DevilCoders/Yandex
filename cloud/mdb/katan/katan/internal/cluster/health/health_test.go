package health

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
)

func TestLastEvent(t *testing.T) {

	t.Run("for empty events last is zero", func(t *testing.T) {
		require.Equal(t, juggler.RawEvent{}, LastEvent(nil))
	})
	t.Run("last", func(t *testing.T) {
		now := time.Now()
		yesterday := now.Add(-time.Hour * 24)

		require.Equal(
			t,
			juggler.RawEvent{
				ReceivedTime: now,
				Status:       "OK",
			},
			LastEvent([]juggler.RawEvent{
				{
					ReceivedTime: yesterday,
					Status:       "CRIT",
				},
				{
					ReceivedTime: now,
					Status:       "OK",
				},
			}),
		)
	})
}

func TestServicesByTags(t *testing.T) {
	t.Run("for custom clusters returns meta", func(t *testing.T) {
		require.Equal(t, []string{"META"}, ServicesByTags(tags.ClusterTags{}))
	})
	t.Run("for postgres", func(t *testing.T) {
		require.Equal(t, []string{"META", "pg_ping"}, ServicesByTags(tags.ClusterTags{
			Meta: tags.ClusterMetaTags{
				Type: "postgresql_cluster",
			},
		}))
	})
}

func TestManagedByHealth(t *testing.T) {
	t.Run("meta cluster", func(t *testing.T) {
		require.True(t, ManagedByHealth(tags.ClusterTags{
			Version: 1,
			Source:  tags.MetaDBSource,
			Meta: tags.ClusterMetaTags{
				Type: "postgresql_cluster",
				Env:  "qa",
				Rev:  42,
			},
		}))
	})

	t.Run("not meta clusters", func(t *testing.T) {
		require.False(t, ManagedByHealth(tags.ClusterTags{
			Version: 1,
			Source:  "conductor",
			Meta:    tags.ClusterMetaTags{},
		}))
	})
}
