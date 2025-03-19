package yacrt

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestNew(t *testing.T) {
	t.Run("OAuth should be specified", func(t *testing.T) {
		_, err := New(&nop.Logger{}, Config{})
		require.Error(t, err)
	})
}
