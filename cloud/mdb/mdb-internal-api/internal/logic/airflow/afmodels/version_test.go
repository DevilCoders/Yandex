package afmodels

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestValidVersion(t *testing.T) {
	ver, err := FindVersion("2.2.4")
	require.NoError(t, err)
	require.Equal(t, "2.2.4", ver.Name)
	require.Equal(t, 2, ver.Major)
	require.Equal(t, 2, ver.Minor)
	require.Equal(t, 4, ver.Patch)
}

func TestVersionsVisibleInConsole(t *testing.T) {
	for _, version := range Versions {
		require.Contains(t, VersionsVisibleInConsole, version.Name)
	}
}

func TestUnknownVersion(t *testing.T) {
	_, err := FindVersion("2.7")
	require.EqualError(t, err, "Unsupported Airflow version")
}
