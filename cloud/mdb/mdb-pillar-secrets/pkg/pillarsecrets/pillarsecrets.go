package pillarsecrets

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
)

//go:generate ../../../scripts/mockgen.sh PillarSecretsClient

type PillarSecretsClient interface {
	ready.Checker

	GetClusterPillarSecret(ctx context.Context, cid string, path []string) (secret.String, error)
	GetSubClusterPillarSecret(ctx context.Context, cid, subcid string, path []string) (secret.String, error)
	GetShardPillarSecret(ctx context.Context, cid, shardID string, path []string) (secret.String, error)
	GetHostPillarSecret(ctx context.Context, cid, fqdn string, path []string) (secret.String, error)
}
