package yoignore

import (
	"io/ioutil"
	"os"
	"path/filepath"
	"testing"

	"github.com/davecgh/go-spew/spew"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yo/pkg/testutil"
)

type testCase struct {
	path   string
	dir    bool
	ignore bool
}

func TestIgnoreRules(t *testing.T) {
	ignore, err := Load(testutil.ArcadiaRoot, "vendor/github.com/test")
	require.NoError(t, err)

	tmpDir, err := ioutil.TempDir("", "yoignore")
	require.NoError(t, err)

	for _, test := range []testCase{
		{path: "vendor/github.com/index.php", dir: false, ignore: true},
		{path: "vendor/github.com/nested/index.php.css", dir: false, ignore: false},
		{path: "vendor/github.com/test/doc/index.html", dir: false, ignore: true},
		{path: "vendor/github.com/test/main.csharp", dir: false, ignore: false},
		{path: "vendor/github.com/test/csharp", dir: true, ignore: true},
		{path: "vendor/github.com/test/go/doc", dir: true, ignore: true},
		{path: "vendor/github.com/test/contrib/csharp", dir: true, ignore: true},
		{path: "vendor/github.com/test/README.md", dir: false, ignore: true},
		{path: "vendor/github.com/test/doc/README.md", dir: false, ignore: false},
	} {
		assert.Equalf(t, test.ignore, ignore.Ignores(test.path, test.dir), spew.Sdump(test))
	}

	// doc dir should be missing
	require.NoError(t, os.MkdirAll(filepath.Join(tmpDir, "vendor", "github.com", "test"), 0777))
	require.NoError(t, ignore.Save(tmpDir))

	checkExists := func(path string) {
		_, err := os.Stat(path)
		require.NoError(t, err, path)
	}

	checkEqual := func(expected, actual string) {
		a, err := ioutil.ReadFile(expected)
		require.NoError(t, err, expected)

		b, err := ioutil.ReadFile(actual)
		require.NoError(t, err, actual)

		require.Equalf(t, a, b, "%s != %s", expected, actual)
	}

	checkExists(filepath.Join(tmpDir, "vendor", Filename))
	checkEqual(
		filepath.Join(testutil.ArcadiaRoot, "vendor", Filename),
		filepath.Join(tmpDir, "vendor", Filename),
	)

	checkExists(filepath.Join(tmpDir, "vendor", "github.com", "test", Filename))
	checkEqual(
		filepath.Join(testutil.ArcadiaRoot, "vendor", "github.com", "test", Filename),
		filepath.Join(tmpDir, "vendor", "github.com", "test", Filename),
	)

	checkExists(filepath.Join(tmpDir, "vendor", "github.com", "test/doc", Filename))
	checkEqual(
		filepath.Join(testutil.ArcadiaRoot, "vendor", "github.com", "test/doc", Filename),
		filepath.Join(tmpDir, "vendor", "github.com", "test/doc", Filename),
	)
}
