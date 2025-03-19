package rediscachetier

import (
	"context"
	"io/ioutil"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestReelect(t *testing.T) {
	tests := []struct {
		content  string
		isLeader bool
	}{
		{content: "master", isLeader: true},
		{content: "slave", isLeader: false},
		{content: "sentinel", isLeader: false},
		{content: "probablymaster", isLeader: false},
	}
	for _, tc := range tests {
		t.Run(tc.content, func(t *testing.T) {
			cfg := DefaultConfig()
			cfg.turnOffElectCycle = true
			f, err := ioutil.TempFile(t.TempDir(), "redis_cache_tire_*")
			require.NoError(t, err)
			defer f.Close()
			_, err = f.WriteString(tc.content)
			require.NoError(t, err)
			cfg.redisTierCacheFile = f.Name()

			ctx := context.Background()
			logger, _ := zap.New(zap.KVConfig(log.TraceLevel))

			le := New(ctx, cfg, logger)
			le.refresh()
			if tc.isLeader {
				require.True(t, le.IsLeader(ctx))
			} else {
				require.False(t, le.IsLeader(ctx))
			}
		})
	}
}

func TestReelectCycle(t *testing.T) {
	cfg := DefaultConfig()
	cfg.LeaderTTL = time.Microsecond
	f, err := ioutil.TempFile(t.TempDir(), "redis_cache_tire_*")
	require.NoError(t, err)
	defer f.Close()
	_, err = f.WriteString(redisTierMaster)
	require.NoError(t, err)
	cfg.redisTierCacheFile = f.Name()
	err = f.Sync()
	require.NoError(t, err)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	logger, _ := zap.New(zap.KVConfig(log.TraceLevel))

	le := New(ctx, cfg, logger)
	time.Sleep(time.Millisecond)
	require.True(t, le.IsLeader(ctx))
}
