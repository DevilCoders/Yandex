package auth

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/metadb"
)

//go:generate ../../../scripts/mockgen.sh Authenticator

type Authenticator interface {
	ReadOnCluster(ctx context.Context, clusterID string, do ReadOnClusterFunc) error
}

type ReadOnClusterFunc func(context.Context, metadb.MetaDB) error
