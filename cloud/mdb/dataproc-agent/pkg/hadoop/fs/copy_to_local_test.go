package fs

import (
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestCopyToLocalWithoutError(t *testing.T) {
	defer func() { doCopyToLocal = execCopyToLocal }()

	doCopyToLocal = func(sourceURL string, localPath string) error {
		require.Equal(t, "s3a://bucket-name/file.jar", sourceURL)
		require.Equal(t, "/tmp/file.jar", localPath)
		return nil
	}
	err := CopyToLocal("s3a://bucket-name/file.jar", "/tmp/file.jar")
	require.NoError(t, err)
}

func TestCopyToLocalWithError(t *testing.T) {
	defer func() { doCopyToLocal = execCopyToLocal }()

	err := xerrors.New("some error text")
	doCopyToLocal = func(sourceURL string, localPath string) error {
		require.Equal(t, "s3a://bucket-name/file.jar", sourceURL)
		require.Equal(t, "/tmp/file.jar", localPath)
		return err
	}
	err2 := CopyToLocal("s3a://bucket-name/file.jar", "/tmp/file.jar")
	require.Error(t, err2)
	require.Equal(t, err, err2)
}

func TestCopyToLocalHttpError(t *testing.T) {
	defer func() { doCopyToLocal = execCopyToLocal }()

	doCopyToLocal = func(sourceURL string, localPath string) error {
		t.Fatalf("this should not be called")
		return nil
	}
	ts := httptest.NewServer(http.NotFoundHandler())
	err := CopyToLocal(ts.URL+"/file.jar", "/tmp/file.jar")
	require.Error(t, err)
	require.Equal(t, "404 Not Found", err.Error())
}

func TestCopyToLocalOutputAsError(t *testing.T) {
	output := []byte(`19/11/29 13:36:22 INFO Configuration.deprecation: fs.s3a.server-side-encryption-key is deprecated. Instead, use fs.s3a.server-side-encryption.key
-copyToLocal: Can not create a Path from an empty string

Usage: hadoop fs [generic options]

The general command line syntax is:
command [genericOptions] [commandOptions]

Usage: hadoop fs [generic options] -copyToLocal [-f] [-p] [-ignoreCrc] [-crc] <src> ... <localdst>`)
	err := copyToLocalOutputAsError(output)
	require.Error(t, err)
	require.Equal(t, "Can not create a Path from an empty string", err.Error())
}
