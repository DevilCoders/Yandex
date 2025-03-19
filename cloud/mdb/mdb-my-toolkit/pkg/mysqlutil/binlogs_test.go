package mysqlutil

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestNextBinlog(t *testing.T) {
	require.Equal(t, "qwe.000001", NextBinlog("qwe.000000"), "NextBinlog ok qwe.000000")
	require.Equal(t, "qwe.000100", NextBinlog("qwe.000099"), "NextBinlog ok qwe.000099")
	require.Equal(t, "qwe.1000000", NextBinlog("qwe.999999"), "NextBinlog ok qwe.999999")
}

func TestPrevBinlog(t *testing.T) {
	require.Equal(t, "qwe.000000", PrevBinlog("qwe.000000"), "PrevBinlog ok qwe.000000")
	require.Equal(t, "qwe.000098", PrevBinlog("qwe.000099"), "PrevBinlog ok qwe.000099")
	require.Equal(t, "qwe.999999", PrevBinlog("qwe.1000000"), "PrevBinlog ok qwe.1000000")
}
