package internal

import (
	"context"
	"sync"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys"
	skmocks "a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestPrivateNewMinionPinger(t *testing.T) {
	ctx, cfg, client, auth, skeys, l := initTestMinionPingerMocks(t)
	mp := newMinionPinger(ctx, cfg, client, auth, skeys, l)
	require.NotNil(t, mp)
	require.NotNil(t, mp.ch)
	require.NotNil(t, mp.runningPings)
}

func TestPublicNewMinionPinger(t *testing.T) {
	ctx, cfg, client, auth, skeys, l := initTestMinionPingerMocks(t)
	var cancel context.CancelFunc
	ctx, cancel = context.WithCancel(ctx)
	defer cancel()
	skeys.EXPECT().List(saltkeys.Accepted).Return(nil, nil).AnyTimes()
	mp := NewMinionPinger(ctx, cfg, client, auth, skeys, l)
	require.NotNil(t, mp)
	require.NotNil(t, mp.ch)
	require.NotNil(t, mp.runningPings)
}

func TestPingMinion(t *testing.T) {
	ctx, cfg, client, auth, skeys, l := initTestMinionPingerMocks(t)
	mp := newMinionPinger(ctx, cfg, client, auth, skeys, l)

	fqdn := "fqdn"
	mp.runningPings.Store(fqdn, struct{}{})
	client.EXPECT().Run(gomock.Any(), auth, 5*time.Second, fqdn, minionPingFunc).Times(1).Return(nil)
	mp.pingMinion(fqdn)
	_, ok := mp.runningPings.Load(fqdn)
	require.False(t, ok)
}

func TestDoPings(t *testing.T) {
	ctx, cfg, client, auth, skeys, l := initTestMinionPingerMocks(t)
	mp := newMinionPinger(ctx, cfg, client, auth, skeys, l)

	// Setup data
	fqdns := []string{"fqdn_1", "fqdn_2", "fqdn_3"}

	// Return list of fqdns on first call and then return nothing
	firstCall := skeys.EXPECT().List(saltkeys.Accepted).Return(fqdns, nil)
	skeys.EXPECT().List(saltkeys.Accepted).Return([]string{}, nil).After(firstCall).AnyTimes()

	// Initiate pings
	go mp.doPings()

	// Make a set of fqdns to test against
	waiting := make(map[string]struct{}, len(fqdns))
	for _, fqdn := range fqdns {
		waiting[fqdn] = struct{}{}
	}
	require.Len(t, waiting, len(fqdns))

	// Set timeout - 10 seconds should be more than enough
	var cancel context.CancelFunc
	ctx, cancel = context.WithTimeout(ctx, time.Second*10)
	defer cancel()
	// Retrieve fqdns from channel
	for {
		if len(waiting) == 0 {
			break
		}

		select {
		case <-ctx.Done():
			t.Fatalf("error while waiting for fqdns: %s", ctx.Err())
		case fqdn := <-mp.ch:
			_, ok := waiting[fqdn]
			require.True(t, ok)
			delete(waiting, fqdn)
		}
	}
}

func TestMinionPinger(t *testing.T) {
	ctx, cfg, client, auth, skeys, l := initTestMinionPingerMocks(t)

	// Setup data
	wg := &sync.WaitGroup{}
	fqdns := []string{"fqdn_1", "fqdn_2", "fqdn_3"}

	// Return list of fqdns on first call and then return nothing
	firstCall := skeys.EXPECT().List(saltkeys.Accepted).Return(fqdns, nil)
	skeys.EXPECT().List(saltkeys.Accepted).Return(nil, nil).After(firstCall).AnyTimes()

	// Set ping results for each fqdn
	for _, fqdn := range fqdns {
		f := fqdn
		wg.Add(1)
		client.EXPECT().Run(gomock.Any(), gomock.Any(), 5*time.Second, f, minionPingFunc).Times(1).Return(nil).Do(
			func(arg0, arg1, arg2, arg3, arg4 interface{}, arg5 ...interface{}) { wg.Done() },
		)
	}

	// Create pinger
	var cancel context.CancelFunc
	ctx, cancel = context.WithCancel(ctx)
	defer cancel()
	mp := NewMinionPinger(ctx, cfg, client, auth, skeys, l)
	require.NotNil(t, mp)

	// Wait for it
	wg.Wait()
}

func initTestMinionPingerMocks(t *testing.T) (context.Context, MinionPingerConfig, *mocks.MockClient, *mocks.MockAuth, *skmocks.MockKeys, log.Logger) {
	ctx := context.Background()
	cfg := DefaultMinionPingerConfig()
	cfg.PingPeriod = time.Second
	cfg.MaxParallelPings = 5
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, l)

	ctrl := gomock.NewController(t)
	client := mocks.NewMockClient(ctrl)
	auth := mocks.NewMockAuth(ctrl)
	skeys := skmocks.NewMockKeys(ctrl)
	return ctx, cfg, client, auth, skeys, l
}
