package redis

import (
	"context"
	"time"

	goredis "github.com/go-redis/redis/v8"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
)

// StoreClusterSecret stores cluster secret to Redis
func (b *backend) StoreClusterSecret(ctx context.Context, cid string, secret []byte, timeout time.Duration) error {
	span, _ := opentracing.StartSpanFromContext(ctx, "StoreClusterSecret")
	defer span.Finish()

	key := marshalSecret(cid)

	if err := b.client.Set(ctx, key, secret, timeout).Err(); err != nil {
		return semerr.WrapWithUnavailable(err, redisUnavailableErrText)
	}

	return nil
}

// LoadClusterSecret loads cluster secret from Redis
func (b *backend) LoadClusterSecret(ctx context.Context, cid string) ([]byte, error) {
	span, _ := opentracing.StartSpanFromContext(ctx, "LoadClusterSecret")
	defer span.Finish()

	secret, err := b.slaveClient.Get(ctx, marshalSecret(cid)).Result()
	if err != nil {
		if err == goredis.Nil {
			return nil, semerr.WrapWithNotFound(datastore.ErrSecretNotFound.WithFrame(), datastore.SecretNotFoundErrText)
		}

		return nil, semerr.WrapWithUnavailable(err, redisUnavailableErrText)
	}

	return []byte(secret), nil
}
