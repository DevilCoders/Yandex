package test_errors_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/swagger"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_Errors(t *testing.T) {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)

	c, err := swagger.NewClient("localhost", nil, l)
	require.NoError(t, err)

	t.Run("Unavailable", func(t *testing.T) {
		err := c.UpdateHostHealth(context.Background(), types.HostHealth{}, nil)
		require.Error(t, err)
		require.True(t, semerr.IsUnavailable(err))
	})

	t.Run("DeadlineExceeded", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Nanosecond)
		defer cancel()

		err := c.UpdateHostHealth(ctx, types.HostHealth{}, nil)
		require.Error(t, err)
		require.False(t, semerr.IsUnavailable(err))
		require.True(t, xerrors.Is(err, context.DeadlineExceeded))
	})

	t.Run("Canceled", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		cancel()

		err := c.UpdateHostHealth(ctx, types.HostHealth{}, nil)
		require.Error(t, err)
		require.False(t, semerr.IsUnavailable(err))
		require.True(t, xerrors.Is(err, context.Canceled))
	})
}
