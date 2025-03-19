package redis_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/alicebob/miniredis/v2"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore/redis"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	redisMastername = ""
	redisPassword   = ""
	redisDB         = 0
)

type ctxKey string

var (
	ctxKeyMiniRedis = ctxKey("miniredis")
)

func init() {
	// Allows us to understand which test is running
	fmt.Println("We are running Redis unit test")
}

func initRedisImpl(t *testing.T, badaddr bool) (context.Context, datastore.Backend) {
	ctx := context.Background()
	logger, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, logger)
	logger.Error("test")

	mr, err := miniredis.Run()
	require.NoError(t, err)
	require.NotNil(t, mr)
	ctx = context.WithValue(ctx, ctxKeyMiniRedis, mr)

	addrs := []string{mr.Addr()}
	if badaddr {
		addrs = []string{""}
	}

	ds := redis.New(
		logger,
		redis.Config{
			Addrs:      addrs,
			MasterName: redisMastername,
			Password:   secret.NewString(redisPassword),
			DB:         redisDB,
		},
	)
	require.NotNil(t, ds)

	return ctx, ds
}

func closeRedisImpl(ctx context.Context, ds datastore.Backend) {
	_ = ds.Close()

	mr := ctx.Value(ctxKeyMiniRedis).(*miniredis.Miniredis)
	mr.Close()
}

func fastForwardRedisImpl(ctx context.Context, d time.Duration) {
	mr := ctx.Value(ctxKeyMiniRedis).(*miniredis.Miniredis)
	mr.FastForward(d)
}
