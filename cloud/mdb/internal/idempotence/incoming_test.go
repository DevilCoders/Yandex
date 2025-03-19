package idempotence_test

import (
	"context"
	"testing"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
)

func TestIncomingFromContextEmpty(t *testing.T) {
	ctx := context.Background()
	idemp, ok := idempotence.IncomingFromContext(ctx)
	require.False(t, ok)
	require.Empty(t, idemp)
}

func TestWithIncoming(t *testing.T) {
	ctx := context.Background()
	idemp := idempotence.Incoming{ID: idempotence.Must(), Hash: uuid.Must(uuid.NewV4()).Bytes()}
	with := idempotence.WithIncoming(ctx, idemp)
	require.NotNil(t, with)
	require.NotEqual(t, ctx, with)

	idempFromCtx, ok := idempotence.IncomingFromContext(with)
	require.True(t, ok)
	require.Equal(t, idemp, idempFromCtx)

	_, ok = idempotence.OutgoingFromContext(with)
	require.False(t, ok)
}

func TestWithIncomingExisting(t *testing.T) {
	ctx := context.Background()
	idemp := idempotence.Incoming{ID: idempotence.Must(), Hash: uuid.Must(uuid.NewV4()).Bytes()}
	with := idempotence.WithIncoming(ctx, idemp)

	secondIdemp := idempotence.Incoming{ID: idempotence.Must(), Hash: uuid.Must(uuid.NewV4()).Bytes()}
	secondWith := idempotence.WithIncoming(with, secondIdemp)
	require.Equal(t, with, secondWith)

	idempFromCtx, ok := idempotence.IncomingFromContext(secondWith)
	require.True(t, ok)
	require.Equal(t, idemp, idempFromCtx)
	require.NotEqual(t, secondIdemp, idempFromCtx)
}
