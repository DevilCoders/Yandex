//go:build disabled
// +build disabled

package tvm_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm/tvmtool"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestGetServiceTicket(t *testing.T) {
	lg, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, lg)

	c, err := tvmtool.New("mdb-deploy-api", "rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr", "http://localhost:50001", lg)
	//c, err := tvmtool.New("mdb-deploy-api", "tttttttttttttttttttttttttttttttt", "http://localhost:50002", lg)
	require.NoError(t, err)
	require.NotNil(t, c)

	ctx := context.Background()
	require.NoError(t, c.Ping(ctx))

	token, err := c.GetServiceTicket(ctx, "blackbox")
	require.NoError(t, err)
	require.NotEmpty(t, token)
	t.Logf("%+v", token)

	st, err := c.CheckServiceTicket(ctx, "blackbox", token)
	require.NoError(t, err)
	t.Logf("%+v", st)
}
