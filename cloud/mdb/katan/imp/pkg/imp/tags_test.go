package imp_test

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/katan/imp/pkg/imp"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
)

func TestHostToTagsForHostWithoutShard(t *testing.T) {
	expectedTags := tags.HostTags{
		Geo: "man",
		Meta: tags.HostMeta{
			ShardID:      "",
			SubClusterID: "subcid1",
			Roles:        []string{"zk"},
		},
	}
	host := metadb.Host{
		FQDN:         "man-1.db.yandex.net",
		SubClusterID: "subcid1",
		ShardID:      optional.String{},
		Geo:          "man",
		Roles:        []string{"zk"},
		CreatedAt:    time.Now(),
	}
	require.Equal(t, expectedTags, imp.HostToTags(host))
}

func TestHostToTagsForHostWitShard(t *testing.T) {
	expectedTags := tags.HostTags{
		Geo: "sas",
		Meta: tags.HostMeta{
			ShardID:      "shard_id2",
			SubClusterID: "subcid2",
			Roles:        []string{"mongodb_cluster.mongod"},
		},
	}
	host := metadb.Host{
		FQDN:         "sas-2.db.yandex.net",
		SubClusterID: "subcid2",
		ShardID:      optional.NewString("shard_id2"),
		Geo:          "sas",
		Roles:        []string{"mongodb_cluster.mongod"},
		CreatedAt:    time.Now(),
	}
	require.Equal(t, expectedTags, imp.HostToTags(host))
}

func TestClusterToTags(t *testing.T) {
	require.Equal(t, tags.ClusterTags{
		Version: 2,
		Source:  tags.MetaDBSource,
		Meta: tags.ClusterMetaTags{
			Type:   "postgresql_cluster",
			Env:    "qa",
			Rev:    42,
			Status: "RUNNING",
		},
	}, imp.ClusterToTags(
		metadb.Cluster{
			Name:        "pgcluster",
			Type:        "postgresql_cluster",
			Environment: "qa",
			Visible:     true,
			Status:      "RUNNING",
		},
		metadb.ClusterRev{
			ClusterID: "c1",
			Rev:       42,
		}))
}
