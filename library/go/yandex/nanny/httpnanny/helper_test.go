package httpnanny

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestCheckService(t *testing.T) {
	require.NoError(t, checkServiceID("mt_partner-front--marketpartner-12310_bca7f7b4_sas"))
	require.Error(t, checkServiceID("foo_bar?you=owned"))
}
