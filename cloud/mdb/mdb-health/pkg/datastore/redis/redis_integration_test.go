//go:build integration_test
// +build integration_test

package redis_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore/redis"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func init() {
	// Allows us to understand which test is running
	fmt.Println("We are running Redis integration test")
}

func initRedisImpl(t *testing.T, badaddr bool) (context.Context, datastore.Backend) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))

	cfg, err := redis.LoadConfig(logger)
	require.NoError(t, err)
	require.NotNil(t, cfg)

	if badaddr {
		cfg.Addrs = []string{"localhost:1"}
	}

	ds := redis.New(logger, cfg)
	require.NotNil(t, ds)

	return ctx, ds
}

func closeRedisImpl(ctx context.Context, ds datastore.Backend) {
	_ = ds.Close()
}

func fastForwardRedisImpl(ctx context.Context, d time.Duration) {
	time.Sleep(d)
}
