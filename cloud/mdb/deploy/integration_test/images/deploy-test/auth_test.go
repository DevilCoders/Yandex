package deploy_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	envNameDeployAPINoScopeToken     = "MDB_DEPLOY_API_TOKEN_NO_SCOPE"
	envNameDeployAPINoWhiteListToken = "MDB_DEPLOY_API_TOKEN_NO_WHITE_LIST"
)

func deployAPINoScopeToken() string {
	return envVar(envNameDeployAPINoScopeToken, "noscope")
}

func deployAPINoWhiteListToken() string {
	return envVar(envNameDeployAPINoWhiteListToken, "nowhitelist")
}

func initAuthTest(t *testing.T, token string) (context.Context, deployapi.Client, log.Logger) {
	ctx := context.Background()

	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, l)

	dapi, err := restapi.New(
		deployAPIURL(),
		token,
		nil,
		httputil.TLSConfig{CAFile: caPath()},
		httputil.LoggingConfig{},
		l,
	)
	require.NoError(t, err)
	require.NotNil(t, dapi)

	return ctx, dapi, l
}

func TestSuccessfullAuth(t *testing.T) {
	ctx, dapi, lg := initAuthTest(t, deployAPIToken())
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Second*30)
	defer cancel()

	waitForDeployAPI(ctx, t, dapi, lg)

	_, _, err := dapi.GetMasters(ctx, deployapi.Paging{})
	require.NoError(t, err)
}

func TestNoScopeFailAuth(t *testing.T) {
	ctx, dapi, lg := initAuthTest(t, deployAPINoScopeToken())
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Second*30)
	defer cancel()

	waitForDeployAPI(ctx, t, dapi, lg)

	_, _, err := dapi.GetMasters(ctx, deployapi.Paging{})
	require.Error(t, err)
}

func TestNoWhiteListFailAuth(t *testing.T) {
	ctx, dapi, lg := initAuthTest(t, deployAPINoWhiteListToken())
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Second*30)
	defer cancel()

	waitForDeployAPI(ctx, t, dapi, lg)

	_, _, err := dapi.GetMasters(ctx, deployapi.Paging{})
	require.Error(t, err)
}
