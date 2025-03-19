package metadb

import (
	"context"
	"io"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

//go:generate ../../../scripts/mockgen.sh MetaDB

// MetaDB API
type MetaDB interface {
	ready.Checker
	io.Closer

	CloudsWithRunningClusters(ctx context.Context, excludeTagged bool) (ClusterIDsByCloudID, error)
	Clusters(ctx context.Context, clusterIDs ClusterIDs) ([]Cluster, error)
}
