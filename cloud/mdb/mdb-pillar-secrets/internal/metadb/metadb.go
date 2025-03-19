package metadb

import (
	"context"
	"io"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/crypto"
)

//go:generate ../../../scripts/mockgen.sh MetaDB

type MetaDB interface {
	io.Closer
	ready.Checker

	sqlutil.TxBinder

	FolderCoordsByClusterID(ctx context.Context, cloudID string) (string, int64, error)

	ClusterIDBySubClusterID(ctx context.Context, subcid string) (string, error)
	ClusterIDByShardID(ctx context.Context, shardid string) (string, error)
	ClusterIDByFQDN(ctx context.Context, fqdn string) (string, error)

	ClusterPillarKey(ctx context.Context, cid string, path []string) (crypto.CryptoKey, error)
	SubClusterPillarKey(ctx context.Context, subcid string, path []string) (crypto.CryptoKey, error)
	ShardPillarKey(ctx context.Context, shardid string, path []string) (crypto.CryptoKey, error)
	HostPillarKey(ctx context.Context, fqdn string, path []string) (crypto.CryptoKey, error)
}
