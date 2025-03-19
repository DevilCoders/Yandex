package internal

import (
	"context"
	"sync"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys"
	skmocks "a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys/mocks"
	fsmocks "a.yandex-team.ru/cloud/mdb/internal/fs/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestMinionsCache(t *testing.T) {
	cache := newMinionsCache()
	const (
		exists  = "exists"
		missing = "missing"
	)
	minion := models.Minion{FQDN: exists}

	_, ok := cache.Minion(exists)
	require.False(t, ok)
	_, ok = cache.Minion(missing)
	require.False(t, ok)

	cache.StoreMinions(map[string]models.Minion{exists: minion})
	res, ok := cache.Minion(exists)
	require.True(t, ok)
	require.Equal(t, minion, res)
	_, ok = cache.Minion(missing)
	require.False(t, ok)

	cache.InvalidateMinion(exists)
	_, ok = cache.Minion(exists)
	require.False(t, ok)
	_, ok = cache.Minion(missing)
	require.False(t, ok)
}

func TestMinionsCacheConcurrent(t *testing.T) {
	const (
		exists  = "exists"
		missing = "missing"
	)
	minion := models.Minion{FQDN: exists}

	cache := newMinionsCache()
	wg := &sync.WaitGroup{}
	ctx := context.Background()
	ctx, cancel := context.WithTimeout(ctx, time.Second*10)
	defer cancel()

	for i := 0; i < 10; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()

			for {
				select {
				case <-ctx.Done():
					return
				default:
					cache.StoreMinions(map[string]models.Minion{exists: minion})
					cache.Minion(exists)
					cache.Minion(missing)
					cache.InvalidateMinion(exists)
				}
			}
		}()
	}
	wg.Wait()
}

func TestPrivateNewKeysManager(t *testing.T) {
	ctx, cfg, dapi, skeys, watcher, l := initTestKeysManagerMocks(t)
	km := newKeysManager(ctx, cfg, dapi, skeys, watcher, l)
	require.NotNil(t, km)
}

func TestPublicNewKeysManager(t *testing.T) {
	ctx, cfg, dapi, skeys, watcher, l := initTestKeysManagerMocks(t)
	var cancel context.CancelFunc
	ctx, cancel = context.WithCancel(ctx)
	defer cancel()
	watcher.EXPECT().Add(saltkeys.Pre.KeyPath()).Return(nil)
	watcher.EXPECT().Add(saltkeys.Rejected.KeyPath()).Return(nil)
	watcher.EXPECT().Add(saltkeys.Denied.KeyPath()).Return(nil)
	watcher.EXPECT().Events().Return(nil).AnyTimes()
	watcher.EXPECT().Errors().Return(nil).AnyTimes()
	skeys.EXPECT().List(gomock.Any()).Return(nil, xerrors.New("mock_stopper")).AnyTimes()
	km, err := NewKeysManager(ctx, cfg, dapi, skeys, watcher, l)
	// We do not assert expectations here because they are racy
	require.NoError(t, err)
	require.NotNil(t, km)
}

func TestVerifyMinionKey(t *testing.T) {
	ctx, cfg, dapi, skeys, watcher, l := initTestKeysManagerMocks(t)
	km := newKeysManager(ctx, cfg, dapi, skeys, watcher, l)
	require.NotNil(t, km)

	const (
		fqdn = "fqdn"
		key  = "key"
	)
	errRet := xerrors.New("test error")

	// No key
	skeys.EXPECT().Key(fqdn, saltkeys.Accepted).Return("", errRet)
	_, err := km.verifyMinionKey(ctx, models.Minion{FQDN: fqdn}, saltkeys.Accepted)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, errRet))

	// Register key
	skeys.EXPECT().Key(fqdn, saltkeys.Accepted).Return(key, nil)
	dapi.EXPECT().RegisterMinion(gomock.Any(), fqdn, key).Return(models.Minion{FQDN: fqdn}, nil)
	verified, err := km.verifyMinionKey(ctx, models.Minion{FQDN: fqdn}, saltkeys.Accepted)
	require.NoError(t, err)
	require.Equal(t, models.Minion{FQDN: fqdn}, verified)

	// Fail to register key
	skeys.EXPECT().Key(fqdn, saltkeys.Accepted).Return(key, nil)
	dapi.EXPECT().RegisterMinion(gomock.Any(), fqdn, key).Return(models.Minion{}, errRet)
	_, err = km.verifyMinionKey(ctx, models.Minion{FQDN: fqdn}, saltkeys.Accepted)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, errRet))

	// Registered (same key)
	skeys.EXPECT().Key(fqdn, saltkeys.Accepted).Return(key, nil)
	verified, err = km.verifyMinionKey(ctx, models.Minion{FQDN: fqdn, PublicKey: key}, saltkeys.Accepted)
	require.NoError(t, err)
	require.Equal(t, models.Minion{FQDN: fqdn, PublicKey: key}, verified)

	// Registered (different key)
	skeys.EXPECT().Key(fqdn, saltkeys.Accepted).Return(key+key, nil)
	_, err = km.verifyMinionKey(ctx, models.Minion{FQDN: fqdn, PublicKey: key}, saltkeys.Accepted)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, saltkeys.ErrKeyMismatch))

	// Registered (different key, but allowed to reregister)
	skeys.EXPECT().Key(fqdn, saltkeys.Accepted).Return(key+key, nil)
	dapi.EXPECT().RegisterMinion(gomock.Any(), fqdn, key+key).Return(models.Minion{FQDN: fqdn, PublicKey: key + key}, nil)
	verified, err = km.verifyMinionKey(ctx, models.Minion{FQDN: fqdn, PublicKey: key, RegisterUntil: time.Now()}, saltkeys.Accepted)
	require.NoError(t, err)
	require.Equal(t, models.Minion{FQDN: fqdn, PublicKey: key + key}, verified)
}

func TestOurMinion(t *testing.T) {
	const (
		minion = "minion"
		master = "master"
	)

	errRet := xerrors.New("test error")

	ctx, cfg, dapi, skeys, watcher, l := initTestKeysManagerMocks(t)
	cfg.SaltMasterFQDN = master
	km := newKeysManager(ctx, cfg, dapi, skeys, watcher, l)
	require.NotNil(t, km)

	// Unknown minion
	dapi.EXPECT().GetMinion(gomock.Any(), minion).Return(models.Minion{}, deployapi.ErrNotFound)
	_, ok, err := km.ourMinion(ctx, minion)
	require.NoError(t, err)
	require.False(t, ok)

	// Error getting minion
	dapi.EXPECT().GetMinion(gomock.Any(), minion).Return(models.Minion{}, errRet)
	_, _, err = km.ourMinion(ctx, minion)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, errRet))

	// Wrong master
	dapi.EXPECT().GetMinion(gomock.Any(), minion).Return(models.Minion{FQDN: minion, MasterFQDN: master + master}, nil)
	_, ok, err = km.ourMinion(ctx, minion)
	require.NoError(t, err)
	require.False(t, ok)

	// Minion is deleted
	dapi.EXPECT().GetMinion(gomock.Any(), minion).Return(models.Minion{FQDN: minion, MasterFQDN: master, Deleted: true}, nil)
	_, ok, err = km.ourMinion(ctx, minion)
	require.NoError(t, err)
	require.False(t, ok)

	// Success
	dapi.EXPECT().GetMinion(gomock.Any(), minion).Return(models.Minion{FQDN: minion, MasterFQDN: master}, nil)
	m, ok, err := km.ourMinion(ctx, minion)
	require.NoError(t, err)
	require.True(t, ok)
	require.Equal(t, minion, m.FQDN)
	require.Equal(t, master, m.MasterFQDN)

	// Cached minion
	minions := map[string]models.Minion{minion: models.Minion{FQDN: minion}}
	km.minions.StoreMinions(minions)
	m, ok, err = km.ourMinion(ctx, minion)
	require.NoError(t, err)
	require.True(t, ok)
	require.Equal(t, minion, m.FQDN)
}

func TestCleanupKeyStates(t *testing.T) {
	inputs := []struct {
		name     string
		rejected []string
		denied   []string
	}{
		{
			name: "empty minions",
		},
		{
			name:     "has rejected",
			rejected: []string{"rejected1", "rejected2"},
		},
		{
			name:   "has denied",
			denied: []string{"denied1", "denied2"},
		},
		{
			name:     "has both",
			rejected: []string{"rejected1", "rejected2"},
			denied:   []string{"denied1", "denied2"},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			ctx, cfg, dapi, skeys, watcher, l := initTestKeysManagerMocks(t)
			km := newKeysManager(ctx, cfg, dapi, skeys, watcher, l)
			require.NotNil(t, km)

			// Specify returned minions
			skeys.EXPECT().List(saltkeys.Rejected).Return(input.rejected, nil)
			skeys.EXPECT().List(saltkeys.Denied).Return(input.denied, nil)

			// All returned minions must be deleted
			for _, fqdn := range append(input.rejected[:], input.denied...) {
				skeys.EXPECT().Delete(fqdn).Return(nil)
			}

			km.cleanupKeyStates()
		})
	}
}

func TestActualizeKeys(t *testing.T) {
	inputs := []struct {
		name   string
		known  map[string][]saltkeys.StateID
		loaded map[string]models.Minion
		setup  func(skeys *skmocks.MockKeys)
	}{
		{
			name:   "empty args",
			known:  map[string][]saltkeys.StateID{},
			loaded: map[string]models.Minion{},
		},
		{
			name: "has known, empty loaded",
			known: map[string][]saltkeys.StateID{
				"pre":      {saltkeys.Pre},
				"accepted": {saltkeys.Accepted},
			},
			loaded: map[string]models.Minion{},
			setup: func(skeys *skmocks.MockKeys) {
				skeys.EXPECT().Delete("pre").Return(nil)
				skeys.EXPECT().Delete("accepted").Return(nil)
			},
		},
		{
			name:  "empty known, has loaded",
			known: map[string][]saltkeys.StateID{},
			loaded: map[string]models.Minion{
				"fqdn": {FQDN: "fqdn"},
			},
		},
		{
			name: "known pre and ours, good key",
			known: map[string][]saltkeys.StateID{
				"fqdn": {saltkeys.Pre},
			},
			loaded: map[string]models.Minion{
				"fqdn": {FQDN: "fqdn", PublicKey: "key"},
			},
			setup: func(skeys *skmocks.MockKeys) {
				skeys.EXPECT().Key("fqdn", saltkeys.Pre).Return("key", nil)
				skeys.EXPECT().Accept("fqdn").Return(nil)
			},
		},
		{
			name: "known pre and ours, bad key",
			known: map[string][]saltkeys.StateID{
				"fqdn": {saltkeys.Pre},
			},
			loaded: map[string]models.Minion{
				"fqdn": {FQDN: "fqdn", PublicKey: "key"},
			},
			setup: func(skeys *skmocks.MockKeys) {
				skeys.EXPECT().Key("fqdn", saltkeys.Pre).Return("bad", nil)
				skeys.EXPECT().Delete("fqdn").Return(nil)
			},
		},
		{
			name: "known accepted and ours, good key",
			known: map[string][]saltkeys.StateID{
				"fqdn": {saltkeys.Accepted},
			},
			loaded: map[string]models.Minion{
				"fqdn": {FQDN: "fqdn", PublicKey: "key"},
			},
			setup: func(skeys *skmocks.MockKeys) {
				skeys.EXPECT().Key("fqdn", saltkeys.Accepted).Return("key", nil)
			},
		},
		{
			name: "known accepted and ours, bad key",
			known: map[string][]saltkeys.StateID{
				"fqdn": {saltkeys.Accepted},
			},
			loaded: map[string]models.Minion{
				"fqdn": {FQDN: "fqdn", PublicKey: "key"},
			},
			setup: func(skeys *skmocks.MockKeys) {
				skeys.EXPECT().Key("fqdn", saltkeys.Accepted).Return("bad", nil)
				skeys.EXPECT().Delete("fqdn").Return(nil)
			},
		},
		{
			name: "known and ours, multiple states",
			known: map[string][]saltkeys.StateID{
				"fqdn": {saltkeys.Accepted, saltkeys.Pre},
			},
			loaded: map[string]models.Minion{
				"fqdn": {FQDN: "fqdn"},
			},
			setup: func(skeys *skmocks.MockKeys) {
				skeys.EXPECT().Delete("fqdn").Return(nil)
			},
		},
		{
			name: "known and not ours, multiple states",
			known: map[string][]saltkeys.StateID{
				"fqdn": {saltkeys.Accepted, saltkeys.Pre},
			},
			setup: func(skeys *skmocks.MockKeys) {
				skeys.EXPECT().Delete("fqdn").Return(nil)
			},
		},
		{
			name: "known and not ous, zero states",
			known: map[string][]saltkeys.StateID{
				"fqdn": {},
			},
			setup: func(skeys *skmocks.MockKeys) {
				skeys.EXPECT().Delete("fqdn").Return(nil)
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			ctx, cfg, dapi, skeys, watcher, l := initTestKeysManagerMocks(t)
			km := newKeysManager(ctx, cfg, dapi, skeys, watcher, l)
			require.NotNil(t, km)

			if input.setup != nil {
				input.setup(skeys)
			}

			km.actualizeKeys(ctx, input.known, input.loaded)
		})
	}
}

func initTestKeysManagerMocks(t *testing.T) (context.Context, KeysManagerConfig, *mocks.MockClient, *skmocks.MockKeys, *fsmocks.MockWatcher, log.Logger) {
	ctx := context.Background()
	cfg := DefaultKeysManagerConfig()
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, l)

	ctrl := gomock.NewController(t)
	dapi := mocks.NewMockClient(ctrl)
	skeys := skmocks.NewMockKeys(ctrl)
	watcher := fsmocks.NewMockWatcher(ctrl)
	return ctx, cfg, dapi, skeys, watcher, l
}
