package imp

import (
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
)

// HostToTags create host tags from metadb host
func HostToTags(host metadb.Host) tags.HostTags {
	return tags.HostTags{
		Geo: host.Geo,
		Meta: tags.HostMeta{
			ShardID:      host.ShardID.String,
			SubClusterID: host.SubClusterID,
			Roles:        host.Roles,
		},
	}
}

func ClusterToTags(cluster metadb.Cluster, rev metadb.ClusterRev) tags.ClusterTags {
	return tags.ClusterTags{
		Version: TagsVersion,
		Source:  tags.MetaDBSource,
		Meta: tags.ClusterMetaTags{
			Type:   cluster.Type,
			Env:    cluster.Environment,
			Rev:    rev.Rev,
			Status: cluster.Status,
		},
	}
}
