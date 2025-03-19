package redsync

import (
	"context"
	"testing"
	"time"

	"github.com/alicebob/miniredis/v2"
	"github.com/go-redis/redis/v8"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func setupRedis(t *testing.T) (redis.UniversalClient, func(t *testing.T)) {
	minired, err := miniredis.Run()
	require.NoError(t, err)
	opts := &redis.UniversalOptions{
		Addrs: []string{minired.Addr()},
	}
	c := redis.NewUniversalClient(opts)
	require.NoError(t, c.Ping(context.Background()).Err())
	return c, func(t *testing.T) {
		minired.Close()
	}
}

func TestReelect(t *testing.T) {
	c, teardown := setupRedis(t)
	defer teardown(t)

	cfg := DefaultConfig()
	cfg.turnOffElectCycle = true
	ctx := context.Background()

	logger, _ := zap.New(zap.KVConfig(log.TraceLevel))

	le := New(ctx, c, cfg, logger)
	le.reelect(ctx)
	require.True(t, le.IsLeader(ctx))

	value, err := c.Get(ctx, "app-all-leader-election-lock").Result()
	require.NoError(t, err)
	require.Equal(t, "common", value)
}

func TestReelectForTwo(t *testing.T) {
	c, teardown := setupRedis(t)
	defer teardown(t)

	baseCfg := DefaultConfig()
	baseCfg.turnOffElectCycle = true
	cfg1 := baseCfg
	cfg1.InstanceID = "host1"
	cfg2 := baseCfg
	cfg2.InstanceID = "host2"

	ctx := context.Background()

	logger, _ := zap.New(zap.KVConfig(log.TraceLevel))

	le1 := New(ctx, c, cfg1, logger)
	le2 := New(ctx, c, cfg2, logger)

	le1.reelect(ctx)
	require.True(t, le1.IsLeader(ctx))

	le2.reelect(ctx)
	require.False(t, le2.IsLeader(ctx))
	require.True(t, le1.IsLeader(ctx)) // In case of fail check a time inside redis it can be error-prone.

	value, err := c.Get(ctx, "app-all-leader-election-lock").Result()
	require.NoError(t, err)
	require.Equal(t, "host1", value)
}

func TestReelectWithTimeout(t *testing.T) {
	c, teardown := setupRedis(t)
	defer teardown(t)

	cfg := DefaultConfig()
	cfg.turnOffElectCycle = true
	ctx := context.Background()

	logger, _ := zap.New(zap.KVConfig(log.TraceLevel))

	le := New(ctx, c, cfg, logger)
	le.reelect(ctx)
	require.True(t, le.IsLeader(ctx))

	ok, err := c.Expire(ctx, "app-all-leader-election-lock", 0).Result()
	require.NoError(t, err)
	require.True(t, ok)

	require.False(t, le.IsLeader(ctx))

	le.reelect(ctx)
	require.True(t, le.IsLeader(ctx))
}

func TestReelectWithExtend(t *testing.T) {
	c, teardown := setupRedis(t)
	defer teardown(t)

	cfg := DefaultConfig()
	cfg.turnOffElectCycle = true
	ctx := context.Background()

	logger, _ := zap.New(zap.KVConfig(log.TraceLevel))

	le := New(ctx, c, cfg, logger)
	le.reelect(ctx)
	require.True(t, le.IsLeader(ctx))

	ok, err := c.Expire(ctx, "app-all-leader-election-lock", 3*time.Second).Result()
	require.NoError(t, err)
	require.True(t, ok)

	require.True(t, le.IsLeader(ctx)) // In case of fail check a time inside redis it can be error-prone.

	le.reelect(ctx)
	require.True(t, le.IsLeader(ctx))

	ttl2, err := c.TTL(ctx, "app-all-leader-election-lock").Result()
	require.NoError(t, err)

	// TTL need to be extended
	require.NotEqual(t, ttl2, 3*time.Second)
}

func TestReelectCycle(t *testing.T) {
	c, teardown := setupRedis(t)
	defer teardown(t)

	cfg := DefaultConfig()
	cfg.LeaderTTL = time.Microsecond
	ctx := context.Background()

	logger, _ := zap.New(zap.KVConfig(log.TraceLevel))

	le := New(ctx, c, cfg, logger)
	time.Sleep(time.Microsecond)
	require.False(t, le.IsLeader(ctx))
}
