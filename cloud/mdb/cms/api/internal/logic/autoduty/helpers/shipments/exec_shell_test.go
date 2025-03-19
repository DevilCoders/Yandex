package shipments_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
)

func TestShIfPresent(t *testing.T) {
	t.Run("with args", func(t *testing.T) {
		require.Equal(
			t,
			"if test -f any-script ; then any-script param1 param2; fi",
			shipments.ShExecIfPresent(
				"any-script",
				"param1", "param2",
			),
		)
	})
	t.Run("no args", func(t *testing.T) {
		require.Equal(
			t,
			"if test -f any-script ; then any-script ; fi",
			shipments.ShExecIfPresent(
				"any-script",
			),
		)
	})
}
