package requestid_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/requestid"
)

func TestNew(t *testing.T) {
	rid := requestid.New()
	require.NotEmpty(t, rid)
}

func TestFromEmptyContext(t *testing.T) {
	ctx := context.Background()
	rid, ok := requestid.CheckedFromContext(ctx)
	require.False(t, ok)
	require.Empty(t, rid)

	require.NotEmpty(t, requestid.FromContextOrNew(ctx))
}

func TestToContext(t *testing.T) {
	ctx := context.Background()
	rid := requestid.New()
	ctxWithRID := requestid.WithRequestID(ctx, rid)
	require.NotNil(t, ctxWithRID)
	require.NotEqual(t, ctx, ctxWithRID)

	ridFromCtx, ok := requestid.CheckedFromContext(ctxWithRID)
	require.True(t, ok)
	require.Equal(t, rid, ridFromCtx)

	ridFromCtx = requestid.FromContextOrNew(ctxWithRID)
	require.Equal(t, rid, ridFromCtx)
}

func TestToContextWithPreviousRequestID(t *testing.T) {
	ctx := context.Background()
	rid := requestid.New()
	ctx = requestid.WithRequestID(ctx, rid)

	secondRID := requestid.New()
	require.NotEqual(t, rid, secondRID)
	sameCtx := requestid.WithRequestID(ctx, secondRID)
	require.Equal(t, ctx, sameCtx)

	ridFromCtx, ok := requestid.CheckedFromContext(sameCtx)
	require.True(t, ok)
	require.Equal(t, rid, ridFromCtx)
	require.NotEqual(t, secondRID, ridFromCtx)

	ridFromCtx = requestid.FromContextOrNew(sameCtx)
	require.Equal(t, rid, ridFromCtx)
	require.NotEqual(t, secondRID, ridFromCtx)
}
