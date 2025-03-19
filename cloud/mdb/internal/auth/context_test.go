package auth_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/auth"
)

func TestAuthToken(t *testing.T) {
	const input = "testtoken"
	ctx := context.Background()
	ctx = auth.WithAuthToken(ctx, input)
	token, ok := auth.TokenFromContext(ctx)
	require.True(t, ok)
	require.Equal(t, input, token)
}

func TestNoAuthToken(t *testing.T) {
	ctx := context.Background()
	token, ok := auth.TokenFromContext(ctx)
	require.False(t, ok)
	require.Empty(t, token)
}
