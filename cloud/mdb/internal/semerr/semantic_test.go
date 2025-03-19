package semerr

import (
	"context"
	"fmt"
	"io"
	"net"
	"net/http"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestIsSemanticError(t *testing.T) {
	inputs := []struct {
		Err      error
		Semantic Semantic
		Is       bool
	}{
		{
			Err:      NotImplemented("not implemented"),
			Semantic: SemanticNotImplemented,
			Is:       true,
		},
		{
			Err:      Authentication("authentication"),
			Semantic: SemanticNotImplemented,
		},
		{
			Err:      xerrors.New("foo"),
			Semantic: SemanticNotImplemented,
		},
		{
			Err:      xerrors.Errorf("bar: %w", NotImplemented("not implemented")),
			Semantic: SemanticNotImplemented,
			Is:       true,
		},
		{
			Err:      WrapWithNotImplemented(xerrors.New("bar"), "not implemented"),
			Semantic: SemanticNotImplemented,
			Is:       true,
		},
		{
			Err:      WrapWithUnavailable(xerrors.Errorf("bar: %w", NotImplemented("not implemented")), "unavailable"),
			Semantic: SemanticUnavailable,
			Is:       true,
		},
		{
			Err:      WrapWithUnavailable(xerrors.Errorf("bar: %w", NotImplemented("not implemented")), "unavailable"),
			Semantic: SemanticNotImplemented,
		},
		{
			Err:      WrapWithUnavailable(xerrors.Errorf("bar: %w", NotImplemented("not implemented")), "unavailable"),
			Semantic: SemanticAuthentication,
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("%s/%d/%t", input.Err, input.Semantic, input.Is), func(t *testing.T) {
			assert.Equal(t, input.Is, isSemanticError(input.Err, input.Semantic))
		})
	}
}

func TestAsSemanticError(t *testing.T) {
	inputs := []struct {
		Err error
		As  bool
	}{
		{
			Err: NotImplemented("foo"),
			As:  true,
		},
		{
			Err: Authentication("foo"),
			As:  true,
		},
		{
			Err: xerrors.New("foo"),
		},
		{
			Err: xerrors.Errorf("bar: %w", NotImplemented("foo")),
			As:  true,
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("%+v", input.Err), func(t *testing.T) {
			assert.Equal(t, input.As, AsSemanticError(input.Err) != nil)
		})
	}
}

func TestMultiWrapIs(t *testing.T) {
	err := WrapWithNotFound(
		WrapWithNotImplemented(
			WrapWithUnavailable(
				io.EOF,
				"unavailable",
			),
			"not implemented",
		),
		"not found",
	)
	assert.True(t, xerrors.Is(err, io.EOF))
	assert.False(t, xerrors.Is(err, xerrors.New("random")))

	assert.True(t, IsNotFound(err))
	assert.True(t, isSemanticError(err, SemanticNotFound))

	assert.False(t, IsNotImplemented(err))
	assert.False(t, isSemanticError(err, SemanticNotImplemented))

	assert.False(t, IsUnavailable(err))
	assert.False(t, isSemanticError(err, SemanticUnavailable))

	assert.False(t, IsAuthentication(err))
	assert.False(t, isSemanticError(err, SemanticAuthentication))
}

/*
func TestMultiWrapFormatting(t *testing.T) {
	err := WrapWithNotFound(
		WrapWithNotImplemented(
			WrapWithUnavailable(
				io.EOF,
				"unavailable",
			),
			"not implemented",
		),
		"not found",
	)
	assert.Equal(t, "not found: not implemented: unavailable: EOF", fmt.Sprintf("%s", err))
	assert.Equal(t, "not found: not implemented: unavailable: EOF", fmt.Sprintf("%v", err))

	fullErrorMessage := `not found:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting
        cloud/mdb/internal/semerr/semantic_test.go:155
  - not implemented:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting
        cloud/mdb/internal/semerr/semantic_test.go:156
  - unavailable:
    a.yandex-team.ru/cloud/mdb/internal/semerr.TestMultiWrapFormatting
        cloud/mdb/internal/semerr/semantic_test.go:157
  - EOF`
	assert.Equal(t, fullErrorMessage, fmt.Sprintf("%+v", err))
}
*/
func TestWrapWellKnownChecked_Skip(t *testing.T) {
	err := NotFound("not found")
	e, ok := WrapWellKnownChecked(err)
	require.True(t, ok)
	require.Equal(t, err, e)
	require.True(t, IsNotFound(e))
}

func TestWrapWellKnownChecked_NetError(t *testing.T) {
	t.Run("not context", func(t *testing.T) {
		var d net.Dialer
		_, err := d.DialContext(context.Background(), "tcp", "localhost:12345")
		require.Error(t, err)
		var netErr net.Error
		require.True(t, xerrors.As(err, &netErr))
		require.False(t, xerrors.Is(err, context.DeadlineExceeded))

		e, ok := WrapWellKnownChecked(err)
		require.True(t, ok)
		require.NotEqual(t, err, e)
		require.True(t, IsUnavailable(e))
	})

	t.Run("DeadlineExceeded", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Nanosecond)
		defer cancel()

		var d net.Dialer
		_, err := d.DialContext(ctx, "tcp", "localhost:12345")
		require.Error(t, err)
		var netErr net.Error
		require.True(t, xerrors.As(err, &netErr))
		require.False(t, xerrors.Is(err, context.DeadlineExceeded))

		e, ok := WrapWellKnownChecked(err)
		require.True(t, ok)
		require.NotEqual(t, err, e)
		require.True(t, IsUnavailable(e))
	})

	t.Run("Canceled", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		cancel()

		var d net.Dialer
		_, err := d.DialContext(ctx, "tcp", "localhost:12345")
		require.Error(t, err)
		var netErr net.Error
		require.True(t, xerrors.As(err, &netErr))
		require.False(t, xerrors.Is(err, context.Canceled))

		e, ok := WrapWellKnownChecked(err)
		require.True(t, ok)
		require.NotEqual(t, err, e)
		require.True(t, IsUnavailable(e))
	})
}

func TestWrapWellKnownChecked_NetErrorNetHTTP(t *testing.T) {
	t.Run("not context", func(t *testing.T) {
		r, err := http.NewRequest("GET", "http://localhost:12345", nil)
		require.NoError(t, err)
		_, err = http.DefaultClient.Do(r.WithContext(context.Background()))
		require.Error(t, err)
		var netErr net.Error
		require.True(t, xerrors.As(err, &netErr))
		require.False(t, xerrors.Is(err, context.Canceled))
		require.False(t, xerrors.Is(err, context.DeadlineExceeded))

		e, ok := WrapWellKnownChecked(err)
		require.True(t, ok)
		require.NotEqual(t, err, e)
		require.True(t, IsUnavailable(e))
	})

	t.Run("DeadlineExceeded", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Nanosecond)
		defer cancel()

		r, err := http.NewRequest("GET", "http://localhost:12345", nil)
		require.NoError(t, err)
		_, err = http.DefaultClient.Do(r.WithContext(ctx))
		require.Error(t, err)
		var netErr net.Error
		require.True(t, xerrors.As(err, &netErr))
		require.True(t, xerrors.Is(err, context.DeadlineExceeded))

		e, ok := WrapWellKnownChecked(err)
		require.False(t, ok)
		require.Equal(t, err, e)
		require.False(t, IsUnavailable(e))
	})

	t.Run("Canceled", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		cancel()

		r, err := http.NewRequest("GET", "http://localhost:12345", nil)
		require.NoError(t, err)
		_, err = http.DefaultClient.Do(r.WithContext(ctx))
		require.Error(t, err)
		var netErr net.Error
		require.True(t, xerrors.As(err, &netErr))
		require.True(t, xerrors.Is(err, context.Canceled))

		e, ok := WrapWellKnownChecked(err)
		require.False(t, ok)
		require.Equal(t, err, e)
		require.False(t, IsUnavailable(e))
	})
}
