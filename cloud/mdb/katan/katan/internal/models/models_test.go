package models_test

import (
	"sort"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
)

func TestCluster(t *testing.T) {
	abcCluster := models.Cluster{Hosts: map[string]tags.HostTags{
		"A": {},
		"B": {},
		"C": {},
	}}

	t.Run("FQNDs", func(t *testing.T) {
		fqdns := abcCluster.FQDNs()
		sort.Strings(fqdns)
		require.Equal(t, []string{"A", "B", "C"}, fqdns)
	})

}
