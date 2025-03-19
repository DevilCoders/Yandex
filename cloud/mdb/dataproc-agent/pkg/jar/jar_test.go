package jar

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestReadJarManifest(t *testing.T) {
	m, err := ReadFile("dataproc-examples-1.0-with-main-class.jar")

	require.NoError(t, err)
	require.NotNil(t, m)

	require.Equal(t, "1.0", m["Manifest-Version"])
}

func TestMainClass(t *testing.T) {
	class, err := MainClass("dataproc-examples-1.0-with-main-class.jar")

	require.NoError(t, err)
	require.Equal(t, "ru.yandex.cloud.dataproc.examples.PopulationMRJob", class)
}

func TestParsingErrors(t *testing.T) {
	_, err := ReadFile("broken.jar")
	require.Error(t, err)
	require.Equal(t, "zip: not a valid zip file", err.Error())

	_, err = ReadFile("jsr250-api-1.0-broken-manifest.jar")
	require.Error(t, err)
	require.Equal(t, "can't parse manifest file (wrong format)", err.Error())

	_, err = ReadFile("jsr250-api-1.0-no-manifest.jar")
	require.Error(t, err)
	require.Equal(t, "given file is not a JAR file", err.Error())
}
