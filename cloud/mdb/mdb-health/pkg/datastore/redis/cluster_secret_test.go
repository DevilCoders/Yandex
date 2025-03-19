package redis_test

import (
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestSecretStoreAndLoad(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	cid := uuid.Must(uuid.NewV4()).String()
	secret := uuid.Must(uuid.NewV4()).Bytes()
	require.NoError(t, ds.StoreClusterSecret(ctx, cid, secret, secretTimeout))
	loadedSecret, err := ds.LoadClusterSecret(ctx, cid)
	require.NoError(t, err)
	require.Equal(t, secret, loadedSecret)

	loadedSecret, err = ds.LoadClusterSecret(ctx, "unknown_cid")
	require.Error(t, err)
	require.True(t, xerrors.Is(err, datastore.ErrSecretNotFound))
	require.Nil(t, loadedSecret)
}

func TestSecretTimeout(t *testing.T) {
	ctx, ds := initRedis(t)
	defer closeRedis(ctx, ds)

	cid := uuid.Must(uuid.NewV4()).String()
	secret := uuid.Must(uuid.NewV4()).Bytes()
	require.NoError(t, ds.StoreClusterSecret(ctx, cid, secret, time.Second))
	loadedSecret, err := ds.LoadClusterSecret(ctx, cid)
	require.NoError(t, err)
	require.Equal(t, secret, loadedSecret)

	fastForwardRedis(ctx, 2*time.Second)

	loadedSecret, err = ds.LoadClusterSecret(ctx, cid)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, datastore.ErrSecretNotFound))
	require.Nil(t, loadedSecret)
}
