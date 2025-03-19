package healthstore_test

import (
	"context"
	"sync"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func newStore(backends []healthstore.Backend, cfg healthstore.Config) *healthstore.Store {
	l := zap.Must(zap.ConsoleConfig(log.DebugLevel))
	store := healthstore.NewStore(backends, cfg, l)
	return store
}

func TestAsyncStore(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	hh1 := types.NewHostHealthWithStatus("cid1", "fqdn1", nil, types.HostStatusAlive)
	hh2 := types.NewHostHealthWithStatus("cid2", "fqdn2", nil, types.HostStatusAlive)

	back1 := mocks.NewMockBackend(ctrl)
	back2 := mocks.NewMockBackend(ctrl)
	for _, back := range []*mocks.MockBackend{back1, back2} {
		back.EXPECT().StoreHostsHealth(gomock.Any(), gomock.Any()).DoAndReturn(func(_ context.Context, hostsHealth []healthstore.HostHealthToStore) error {
			require.ElementsMatch(t, hostsHealth, []healthstore.HostHealthToStore{{Health: hh1}, {Health: hh2}})
			return nil
		}).Times(1)
		back.EXPECT().Name().Return("").AnyTimes()
	}

	cfg := healthstore.DefaultConfig()
	cfg.MaxCycles = 2
	store := newStore([]healthstore.Backend{back1, back2}, cfg)

	wg := &sync.WaitGroup{}
	wg.Add(2)
	go func() {
		require.NoError(t, store.StoreHostHealth(ctx, hh1, 0))
		wg.Done()
	}()
	go func() {
		require.NoError(t, store.StoreHostHealth(ctx, hh2, 0))
		wg.Done()
	}()
	wg.Wait()

	store.Run(ctx)
}

func TestAsyncStoreWithErrors(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	hh := types.NewHostHealthWithStatus("cid1", "fqdn1", nil, types.HostStatusAlive)

	back1 := mocks.NewMockBackend(ctrl)
	back1.EXPECT().StoreHostsHealth(gomock.Any(), []healthstore.HostHealthToStore{{Health: hh}}).Return(nil).Times(1)
	back1.EXPECT().Name().Return("good backend").AnyTimes()
	back2 := mocks.NewMockBackend(ctrl)
	back2.EXPECT().StoreHostsHealth(gomock.Any(), []healthstore.HostHealthToStore{{Health: hh}}).Return(semerr.Internal("some error")).Times(1)
	back2.EXPECT().Name().Return("error backend").AnyTimes()

	cfg := healthstore.DefaultConfig()
	cfg.MaxCycles = 2
	store := newStore([]healthstore.Backend{back1, back2}, cfg)

	require.NoError(t, store.StoreHostHealth(ctx, hh, 0))

	store.Run(ctx)
}

func TestInfinitiveLoop(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	cfg := healthstore.DefaultConfig()
	cfg.MaxWaitTime = encodingutil.FromDuration(time.Millisecond)
	store := newStore(nil, cfg)
	store.Run(ctx)
	require.True(t, xerrors.Is(ctx.Err(), context.DeadlineExceeded))
}

func TestBuffSizeOverflow(t *testing.T) {
	ctx := context.Background()
	hh := types.NewHostHealthWithStatus("cid1", "fqdn1", nil, types.HostStatusAlive)
	cfg := healthstore.Config{
		MinBufSize:  1,
		MaxBuffSize: 1,
	}
	store := newStore(nil, cfg)
	require.NoError(t, store.StoreHostHealth(ctx, hh, 0))
	require.True(t, semerr.IsUnavailable(store.StoreHostHealth(ctx, hh, 0)))
}
