package blackbox_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
)

func TestUnauthorizedError_wrap(t *testing.T) {
	statusErr := blackbox.StatusError{
		Status:  blackbox.StatusNoAuth,
		Message: "no auth",
	}

	unauthorizedErr := &blackbox.UnauthorizedError{
		StatusError: statusErr,
	}

	t.Run("UnauthorizedError", func(t *testing.T) {
		var actualErr *blackbox.UnauthorizedError
		ok := xerrors.As(unauthorizedErr, &actualErr)
		require.True(t, ok)
		require.Equal(t, unauthorizedErr.Message, actualErr.Message)
	})

	t.Run("StatusError", func(t *testing.T) {
		var actualErr blackbox.StatusError
		ok := xerrors.As(unauthorizedErr, &actualErr)
		require.True(t, ok)
		require.Equal(t, unauthorizedErr.Message, actualErr.Message)

		ok = xerrors.Is(unauthorizedErr, statusErr)
		require.True(t, ok)
	})
}
