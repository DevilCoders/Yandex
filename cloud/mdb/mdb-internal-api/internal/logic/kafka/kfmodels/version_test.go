package kfmodels

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestValidVersion(t *testing.T) {
	ver, err := FindVersion("2.8")
	require.NoError(t, err)
	require.Equal(t, "2.8", ver.Name)
	require.Equal(t, 2, ver.Major)
	require.Equal(t, 8, ver.Minor)
}

func TestVersionsVisibleInConsole(t *testing.T) {
	for _, version := range Versions {
		require.Contains(t, NamesOfVersionsVisibleInConsoleMDB, version.Name)
	}
}

func TestUnknownVersion(t *testing.T) {
	_, err := FindVersion("2.7")
	require.EqualError(t, err, "unknown Apache Kafka version")
}

func TestValidateUpgradeAllowed(t *testing.T) {
	err := ValidateUpgrade("2.6", "2.8")
	require.NoError(t, err)
}

func TestValidateUpgradeNotAllowed(t *testing.T) {
	err := ValidateUpgrade("2.6", "2.7")
	require.EqualError(t, err, "unknown Apache Kafka version")

	err = ValidateUpgrade("2.8", "2.6")
	require.EqualError(t, err, "downgrade is not allowed")
}

func TestIsSupportedVersion3x(t *testing.T) {
	testData := []struct {
		version              string
		isSupported3xVersion bool
	}{
		{"2.9", false},
		{"fake", false},
		{"3.9", false},
		{"4.0", false},
		{"4.1", false},
		{"2.1", false},
		{"2.6", false},
		{"2.8", false},
		{"3.0", true},
		{"3.1", true},
	}
	for _, data := range testData {
		require.Equal(t, data.isSupported3xVersion, IsSupportedVersion3x(data.version))
	}
}
