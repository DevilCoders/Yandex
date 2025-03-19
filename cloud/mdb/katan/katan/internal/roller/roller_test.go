package roller_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/roller"
)

func TestGroupByClusters(t *testing.T) {
	t.Run("empty hosts and clusters", func(t *testing.T) {
		ret, err := roller.GroupByClusters(nil, nil)
		require.NoError(t, err)
		require.Empty(t, ret)
	})
	t.Run("2 clusters", func(t *testing.T) {
		ret, err := roller.GroupByClusters(
			[]katandb.Cluster{
				{
					ID:   "c1",
					Tags: "{}",
				},
				{
					ID:   "c2",
					Tags: "{}",
				},
			},
			[]katandb.Host{
				{
					ClusterID: "c2",
					FQDN:      "c2-f1",
					Tags:      "{}",
				},
				{
					ClusterID: "c2",
					FQDN:      "c2-f2",
					Tags:      "{}",
				},
				{
					ClusterID: "c1",
					FQDN:      "c1-f1",
					Tags:      "{}",
				},
			},
		)
		require.NoError(t, err)
		require.Len(t, ret, 2)
		require.Contains(t, ret, models.Cluster{
			ID:   "c1",
			Tags: tags.ClusterTags{},
			Hosts: map[string]tags.HostTags{
				"c1-f1": {},
			},
		})
		require.Contains(t, ret, models.Cluster{
			ID:   "c2",
			Tags: tags.ClusterTags{},
			Hosts: map[string]tags.HostTags{
				"c2-f1": {},
				"c2-f2": {},
			},
		})
	})
	t.Run("exists host that not present in clusters", func(t *testing.T) {
		ret, err := roller.GroupByClusters(
			[]katandb.Cluster{
				{
					ID:   "c1",
					Tags: "{}",
				},
			},
			[]katandb.Host{
				{
					ClusterID: "c2",
					FQDN:      "c2-f1",
					Tags:      "{}",
				},
				{
					ClusterID: "c1",
					FQDN:      "c1-f1",
					Tags:      "{}",
				},
			},
		)
		require.Error(t, err)
		require.Empty(t, ret)
	})
	t.Run("cluster without hosts", func(t *testing.T) {
		ret, err := roller.GroupByClusters(
			[]katandb.Cluster{
				{
					ID:   "c1",
					Tags: "{}",
				},
			},
			[]katandb.Host{},
		)
		require.NoError(t, err)
		require.Equal(t, ret, []models.Cluster{{
			ID:    "c1",
			Tags:  tags.ClusterTags{},
			Hosts: map[string]tags.HostTags{},
		}})
	})
	t.Run("bad cluster tags", func(t *testing.T) {
		_, err := roller.GroupByClusters(
			[]katandb.Cluster{
				{
					ID:   "c1",
					Tags: "<xml-not-supported/>",
				},
			},
			[]katandb.Host{
				{
					ClusterID: "c1",
					FQDN:      "c1-f1",
					Tags:      "{}",
				},
			},
		)
		require.Error(t, err)
	})
	t.Run("bad hosts tags", func(t *testing.T) {
		_, err := roller.GroupByClusters(
			[]katandb.Cluster{
				{
					ID:   "c1",
					Tags: "{}",
				},
			},
			[]katandb.Host{
				{
					ClusterID: "c1",
					FQDN:      "c1-f1",
					Tags:      "<xml-not-supported/>",
				},
			},
		)
		require.Error(t, err)
	})
}
