//go:build integration_test
// +build integration_test

package swagger_test

import (
	"context"
	"io/ioutil"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types/testhelpers"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	envMDBHHost = "MDBH_INTEGRATION_TEST_CLIENT_MDBH_HOST"
	// If CA cert path is provided, test assumes HTTPS transport
	envMDBHCACert            = "MDBH_INTEGRATION_TEST_CLIENT_CA_CERT"
	envMDBHClusterPrivateKey = "MDBH_INTEGRATION_TEST_CLIENT_CLUSTER_PRIVATE_KEY"
)

func TestUnavailableMDBH(t *testing.T) {
	ctx, mdbh, key := initBadClient(t)

	err := mdbh.Ping(ctx)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, client.ErrNotAvailable))

	err = mdbh.Stats(ctx)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, client.ErrNotAvailable))

	hh := testhelpers.NewHostHealth(3, 3)

	err = mdbh.UpdateHostHealth(ctx, hh, key)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, client.ErrNotAvailable))

	hhsLoaded, err := mdbh.GetHostsHealth(ctx, []string{hh.FQDN()})
	require.Error(t, err)
	require.True(t, xerrors.Is(err, client.ErrNotAvailable))
	require.Nil(t, hhsLoaded)
}

func TestAvailableMDBH(t *testing.T) {
	ctx, mdbh, _ := initClient(t)

	require.NoError(t, mdbh.Ping(ctx))
}

func TestStatsMDBH(t *testing.T) {
	ctx, mdbh, _ := initClient(t)

	// TODO add data tests when implemented
	require.NoError(t, mdbh.Stats(ctx))
}

func TestHealthUpdate(t *testing.T) {
	ctx, mdbh, key := initClient(t)

	hh := testhelpers.NewHostHealth(3, 3)

	err := mdbh.UpdateHostHealth(ctx, hh, key)
	require.NoError(t, err)

	hhsLoaded, err := mdbh.GetHostsHealth(ctx, []string{hh.FQDN()})
	require.NoError(t, err)
	require.Len(t, hhsLoaded, 1)
}

func TestHealthUpdateTooOld(t *testing.T) {
	ctx, mdbh, key := initClient(t)

	hh := testhelpers.NewHostHealth(3, 3)

	// Store host's health
	require.NoError(t, mdbh.UpdateHostHealth(ctx, hh, key))

	// Try sending update with the same timestamp (should error out as anything <= timestamp stored in mdb-health
	// for the host is considered 'too old')
	err := mdbh.UpdateHostHealth(ctx, hh, key)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, client.ErrBadRequest))

	// Try older timestamp
	var shu []types.ServiceHealth
	for _, s := range hh.Services() {
		shu = append(
			shu,
			types.NewServiceHealth(
				s.Name(),
				s.Timestamp().Add(-time.Minute),
				s.Status(),
				s.Role(),
				s.ReplicaType(),
				s.ReplicaUpstream(),
				s.ReplicaLag(),
				s.Metrics(),
			),
		)
	}

	err = mdbh.UpdateHostHealth(ctx, types.NewHostHealth(hh.ClusterID(), hh.FQDN(), shu), key)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, client.ErrBadRequest))
}

func initBadClient(t *testing.T) (context.Context, client.MDBHealthClient, *crypto.PrivateKey) {
	return initClientImpl(t, true)
}

func initClient(t *testing.T) (context.Context, client.MDBHealthClient, *crypto.PrivateKey) {
	return initClientImpl(t, false)
}

func initClientImpl(t *testing.T, badclient bool) (context.Context, client.MDBHealthClient, *crypto.PrivateKey) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))

	clusterkeypath := getEnvWithDefault(envMDBHClusterPrivateKey, "../../../_test_private_key.pem")
	ctxlog.Debugf(ctx, logger, "Using '%s' as cluster key path.", clusterkeypath)

	clusterkeybin, err := ioutil.ReadFile(clusterkeypath)
	require.NoError(t, err)
	require.NotNil(t, clusterkeybin)
	key, err := crypto.NewPrivateKey(clusterkeybin)
	require.NoError(t, err)
	require.NotNil(t, key)

	tls := false
	cacertpath := os.Getenv(envMDBHCACert)
	if cacertpath == "" {
		logger.Debug("No CA certificate path provided. Assuming HTTP tranposrt.")
	} else {
		ctxlog.Debugf(ctx, logger, "Using '%s' as CA certificate path. Assuming HTTPS transport.", cacertpath)
		tls = true
	}

	host := os.Getenv(envMDBHHost)
	if badclient {
		host = "localhost:1"
	} else if host == "" {
		if tls {
			host = "localhost:12346"
		} else {
			host = "localhost:12345"
		}
	}
	ctxlog.Debugf(ctx, logger, "Using '%s' as host.", host)

	var mdbh client.MDBHealthClient
	if tls {
		mdbh, err = swagger.NewClientTLS(host, cacertpath, logger, swagger.EnableBodySigning(key))
	} else {
		mdbh, err = swagger.NewClient(host, logger, swagger.EnableBodySigning(key))
	}

	require.NoError(t, err)
	require.NotNil(t, mdbh)

	//this code will wait for app to come online
	timeoutCtx, cancel := context.WithTimeout(context.Background(), time.Minute*2)
	require.NoError(t, ready.Wait(timeoutCtx, ready.CheckerFunc(mdbh.Ping), &ready.DefaultErrorTester{}, time.Second))
	cancel()
	return ctx, mdbh, key
}

func getEnvWithDefault(key, def string) string {
	v := os.Getenv(key)
	if v != "" {
		return v
	}

	return def
}
