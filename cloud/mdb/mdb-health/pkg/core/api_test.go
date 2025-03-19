package core_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	leaderelectormocks "a.yandex-team.ru/cloud/mdb/internal/leaderelection/mocks"
	mdbmocks "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types/testhelpers"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	dsmocks "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	ssmocks "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func initGW(t *testing.T) (context.Context, *dsmocks.MockBackend, *ssmocks.MockBackend, *core.GeneralWard, *gomock.Controller, *healthstore.Store) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	ctrl := gomock.NewController(t)
	dsmock := dsmocks.NewMockBackend(ctrl)
	ssmock := ssmocks.NewMockBackend(ctrl)
	gwCfg := core.DefaultConfig()
	mdb := mdbmocks.NewMockMetaDB(ctrl)
	hsConfig := healthstore.DefaultConfig()
	hsConfig.MaxCycles = 2
	hs := healthstore.NewStore([]healthstore.Backend{dsmock}, hsConfig, logger)
	lemock := leaderelectormocks.NewMockLeaderElector(ctrl)
	gw := core.NewGeneralWard(ctx, logger, gwCfg, dsmock, hs, ssmock, mdb, nil, false, lemock)
	require.NotNil(t, gw)
	return ctx, dsmock, ssmock, gw, ctrl, hs
}

func TestUnknownFQDNs(t *testing.T) {
	ctx, dsmock, _, gw, ctrl, _ := initGW(t)
	defer ctrl.Finish()

	fqdns := []string{uuid.Must(uuid.NewV4()).String(), uuid.Must(uuid.NewV4()).String(), uuid.Must(uuid.NewV4()).String()}

	dsmock.EXPECT().LoadHostsHealth(ctx, fqdns).Return([]types.HostHealth{}, nil)
	hhs, err := gw.LoadHostsHealth(ctx, fqdns)
	require.NoError(t, err)
	require.Len(t, hhs, 0)
}

func TestSignatureVerificationFromDataStore(t *testing.T) {
	ctx, dsmock, _, gw, ctrl, _ := initGW(t)
	defer ctrl.Finish()

	privKey, err := crypto.GeneratePrivateKey()
	require.NoError(t, err)
	require.NotNil(t, privKey)

	hh := testhelpers.NewHostHealth(4, 3)
	data := []byte(fmt.Sprintf("%v", hh))
	signature, err := privKey.HashAndSign(data)
	require.NoError(t, err)
	require.NotNil(t, signature)

	dsmock.EXPECT().LoadClusterSecret(ctx, hh.ClusterID()).Return(privKey.GetPublicKey().EncodeToPKCS1(), nil)
	require.NoError(t, gw.VerifyClusterSignature(ctx, hh.ClusterID(), data, signature))
}

func TestSecretLoadFromSecretsStore(t *testing.T) {
	ctx, dsmock, ssmock, gw, ctrl, _ := initGW(t)
	defer ctrl.Finish()

	privKey, err := crypto.GeneratePrivateKey()
	require.NoError(t, err)
	require.NotNil(t, privKey)

	hh := testhelpers.NewHostHealth(4, 3)
	data := []byte(fmt.Sprintf("%v", hh))
	signature, err := privKey.HashAndSign(data)
	require.NoError(t, err)
	require.NotNil(t, signature)

	dsmock.EXPECT().LoadClusterSecret(ctx, hh.ClusterID()).Return(nil, datastore.ErrSecretNotFound)
	ssmock.EXPECT().LoadClusterSecret(ctx, hh.ClusterID()).Return(privKey.GetPublicKey().EncodeToPKCS1(), nil)
	dsmock.EXPECT().StoreClusterSecret(ctx, hh.ClusterID(), privKey.GetPublicKey().EncodeToPKCS1(), gw.SecretTimeout()).
		Return(nil)
	require.NoError(t, gw.VerifyClusterSignature(ctx, hh.ClusterID(), data, signature))
}

func TestSignatureVerificationFromDataStoreFailure(t *testing.T) {
	ctx, dsmock, _, gw, ctrl, _ := initGW(t)
	defer ctrl.Finish()

	privKey, err := crypto.GeneratePrivateKey()
	require.NoError(t, err)
	require.NotNil(t, privKey)

	hh := testhelpers.NewHostHealth(4, 3)
	data := []byte(fmt.Sprintf("%v", hh))
	signature, err := privKey.HashAndSign(data)
	require.NoError(t, err)
	require.NotNil(t, signature)

	dsmock.EXPECT().LoadClusterSecret(ctx, hh.ClusterID()).Return(append(privKey.GetPublicKey().EncodeToPKCS1(), 0x20), nil)
	err = gw.VerifyClusterSignature(ctx, hh.ClusterID(), data, signature)
	require.Error(t, err)
	require.True(t, semerr.IsAuthentication(err))
}

func TestSignatureVerificationFromSecretsStoreFailure(t *testing.T) {
	ctx, dsmock, ssmock, gw, ctrl, _ := initGW(t)
	defer ctrl.Finish()

	privKey, err := crypto.GeneratePrivateKey()
	require.NoError(t, err)
	require.NotNil(t, privKey)

	hh := testhelpers.NewHostHealth(4, 3)
	data := []byte(fmt.Sprintf("%v", hh))
	signature, err := privKey.HashAndSign(data)
	require.NoError(t, err)
	require.NotNil(t, signature)

	dsmock.EXPECT().LoadClusterSecret(ctx, hh.ClusterID()).Return(nil, datastore.ErrSecretNotFound)
	invalidPublicKey := append(privKey.GetPublicKey().EncodeToPKCS1(), 0x20)
	ssmock.EXPECT().LoadClusterSecret(ctx, hh.ClusterID()).Return(invalidPublicKey, nil)
	dsmock.EXPECT().StoreClusterSecret(ctx, hh.ClusterID(), invalidPublicKey, gw.SecretTimeout()).Return(nil)
	err = gw.VerifyClusterSignature(ctx, hh.ClusterID(), data, signature)
	require.Error(t, err)
	require.True(t, semerr.IsAuthentication(err))
}

func TestWrongSignature(t *testing.T) {
	ctx, dsmock, _, gw, ctrl, _ := initGW(t)
	defer ctrl.Finish()

	privKey, err := crypto.GeneratePrivateKey()
	require.NoError(t, err)
	require.NotNil(t, privKey)

	hh := testhelpers.NewHostHealth(4, 3)
	data := []byte(fmt.Sprintf("%v", hh))
	signature, err := privKey.HashAndSign(data)
	require.NoError(t, err)
	require.NotNil(t, signature)

	signature = append(signature, 0x20)

	dsmock.EXPECT().LoadClusterSecret(ctx, hh.ClusterID()).Return(privKey.GetPublicKey().EncodeToPKCS1(), nil)
	err = gw.VerifyClusterSignature(ctx, hh.ClusterID(), data, signature)
	require.Error(t, err)
	require.True(t, semerr.IsAuthentication(err))
}

func TestStoreAndLoad(t *testing.T) {
	ctx, dsmock, _, gw, ctrl, hs := initGW(t)
	defer ctrl.Finish()

	hh := testhelpers.NewHostHealth(4, 3)

	dsmock.EXPECT().StoreHostsHealth(gomock.Any(), []healthstore.HostHealthToStore{{Health: hh, TTL: gw.HostHealthTimeout()}}).Return(nil)
	dsmock.EXPECT().Name().Return("").AnyTimes()
	var defDur time.Duration
	require.NoError(t, gw.StoreHostHealth(ctx, hh, defDur))
	hs.Run(ctx)

	dsmock.EXPECT().LoadHostsHealth(ctx, []string{hh.FQDN()}).Return([]types.HostHealth{hh}, nil)
	hhs, err := gw.LoadHostsHealth(ctx, []string{hh.FQDN()})
	require.NoError(t, err)
	require.Len(t, hhs, 1)
	require.Contains(t, hhs, hh)
}

func TestSetCustomTTL(t *testing.T) {
	ctx, dsmock, _, gw, ctrl, hs := initGW(t)
	defer ctrl.Finish()

	hh := testhelpers.NewHostHealth(4, 3)

	upTimeout := 3 * gw.HostHealthTimeout()
	dsmock.EXPECT().StoreHostsHealth(gomock.Any(), []healthstore.HostHealthToStore{{Health: hh, TTL: upTimeout}}).Return(nil)
	dsmock.EXPECT().Name().Return("").AnyTimes()
	require.Error(t, gw.StoreHostHealth(ctx, hh, time.Hour), "too huge limit")

	require.NoError(t, gw.StoreHostHealth(ctx, hh, upTimeout))
	hs.Run(ctx)

	dsmock.EXPECT().LoadHostsHealth(ctx, []string{hh.FQDN()}).Return([]types.HostHealth{hh}, nil)
	hhs, err := gw.LoadHostsHealth(ctx, []string{hh.FQDN()})
	require.NoError(t, err)
	require.Len(t, hhs, 1)
	require.Contains(t, hhs, hh)
}
