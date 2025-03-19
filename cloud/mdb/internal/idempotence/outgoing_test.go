package idempotence_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
)

func TestNew(t *testing.T) {
	first, err := idempotence.New()
	require.NoError(t, err)
	require.NotEmpty(t, first)

	second, err := idempotence.New()
	require.NoError(t, err)
	require.NotEmpty(t, second)

	require.NotEqual(t, first, second)
}

func TestMust(t *testing.T) {
	id := idempotence.Must()
	require.NotEmpty(t, id)

	require.NotEqual(t, id, idempotence.Must())
}

func TestOutgoingFromContextEmpty(t *testing.T) {
	ctx := context.Background()
	id, ok := idempotence.OutgoingFromContext(ctx)
	require.False(t, ok)
	require.Empty(t, id)
}

func TestWithOutgoing(t *testing.T) {
	ctx := context.Background()
	id := idempotence.Must()
	with := idempotence.WithOutgoing(ctx, id)
	require.NotNil(t, with)
	require.NotEqual(t, ctx, with)

	idFromCtx, ok := idempotence.OutgoingFromContext(with)
	require.True(t, ok)
	require.Equal(t, id, idFromCtx)

	_, ok = idempotence.IncomingFromContext(with)
	require.False(t, ok)
}

func TestWithOutgoingExisting(t *testing.T) {
	ctx := context.Background()
	id := idempotence.Must()
	with := idempotence.WithOutgoing(ctx, id)

	secondID := idempotence.Must()
	secondWith := idempotence.WithOutgoing(with, secondID)
	require.Equal(t, with, secondWith)

	idFromCtx, ok := idempotence.OutgoingFromContext(secondWith)
	require.True(t, ok)
	require.Equal(t, id, idFromCtx)
	require.NotEqual(t, secondID, idFromCtx)
}
