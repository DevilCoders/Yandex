package sessions

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

func TestWithPreferStandby(t *testing.T) {

	got := WithPreferStandby()
	ns := sqlutil.Primary
	got(&ns)
	require.Equal(t, sqlutil.PreferStandby, ns)
}
