package ospillars

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestSetPlugins(t *testing.T) {
	p := NewCluster()
	err := p.SetPlugins("analysis-icu", "analysis-phonetic")
	require.NoError(t, err, "no error expected on known plugin")
}

func TestSetUnknownPlugin(t *testing.T) {
	p := NewCluster()
	err := p.SetPlugins("analysis-icu", "unknown-plugin")
	require.Error(t, err, "error expected on unknown plugin")
}
