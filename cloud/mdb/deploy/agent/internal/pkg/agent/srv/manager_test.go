package srv_test

import (
	"bytes"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/srv"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func testBaseDir(t *testing.T) string {
	rg := regexp.MustCompile(`[\s\\/]+`)
	subTestDir := rg.ReplaceAllString(t.Name(), "_")
	baseDir, err := filepath.Abs(filepath.Join("test-cases-data", subTestDir))
	require.NoError(t, err, "resolve base testDir")
	_ = os.RemoveAll(baseDir)
	require.NoError(t, os.MkdirAll(baseDir, os.ModePerm))
	return baseDir
}

func setupWithScript(t *testing.T, workingDir, script string) {
	cmd := exec.Command("/bin/sh", "-x", "-e", "-c", script)
	cmd.Dir = workingDir
	stdout := &bytes.Buffer{}
	stderr := &bytes.Buffer{}
	cmd.Stdout = stdout
	cmd.Stderr = stderr

	err := cmd.Run()
	require.NoErrorf(t, err, "script fails.\nstdout: %s\nstderr: %s", stdout.String(), stderr.String())
}

func testCfg(baseDir string) srv.Config {
	cfg := srv.DefaultConfig()
	cfg.SrvPath = filepath.Join(baseDir, "srv")
	cfg.ImagesDir = filepath.Join(baseDir, "srv-images")
	if _, err := exec.LookPath(cfg.TarCommand); err != nil {
		// on distbuild PATH not set,
		// so tar without explict path fails
		cfg.TarCommand = "/bin/tar"
	}
	return cfg
}

func TestManager_Version(t *testing.T) {
	t.Run("Good /srv link", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
mkdir -p srv-images/image-123-r123
ln -s $(pwd)/srv-images/image-123-r123 srv
`)
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		v, err := m.Version()
		require.NoError(t, err)
		require.Equal(t, "123-r123", v)
	})

	t.Run("Not initialized /srv", func(t *testing.T) {
		cfg := testCfg(testBaseDir(t))
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		v, err := m.Version()
		require.Empty(t, v)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, agent.ErrSrvNotInitialized))
	})

	t.Run("Malformed /srv (dir instead of symlink)", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `mkdir srv/`)

		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		v, err := m.Version()
		require.Empty(t, v)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, agent.ErrSrvMalformed))
	})

	t.Run("/srv points to not existed dir", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
ln -s $(pwd)/srv-images/image-999-r999 srv
`)
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		v, err := m.Version()
		require.Empty(t, v)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, agent.ErrSrvMalformed))
	})

	t.Run("/srv points to a file", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
mkdir srv-images/
touch srv-images/image-999-r999
ln -s $(pwd)/srv-images/image-999-r999 srv
`)
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		v, err := m.Version()
		require.Empty(t, v)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, agent.ErrSrvMalformed))
	})
}

func openArchive(t *testing.T, baseDir, imageName string) *os.File {
	imageFile, err := os.Open(filepath.Join(baseDir, imageName))
	require.NoError(t, err)
	return imageFile
}

func TestManager_Update(t *testing.T) {
	t.Run("first run /srv not exists yet", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
mkdir image/
echo 'foo: bar' > image/versions.sls
tar -cJvf image-123-r545.txz image
`)
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)
		image := openArchive(t, baseDir, "image-123-r545.txz")
		defer func() { _ = image.Close() }()
		require.NoError(t, m.Update(image))

		v, err := m.Version()
		require.NoError(t, err)
		require.Equal(t, "123-r545", v)
	})

	t.Run("second run /srv update", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
mkdir image/
echo 'foo: bar' > image/versions.sls
tar -cJvf image-123-r123.txz image
`)
		lg, _ := zap.New(zap.CLIConfig(log.DebugLevel))
		m, err := srv.New(cfg, lg)
		require.NoError(t, err)

		image := openArchive(t, baseDir, "image-123-r123.txz")
		defer func() { _ = image.Close() }()
		require.NoError(t, m.Update(image))

		setupWithScript(t, baseDir, `
echo 'wat: man' > image/versions.sls
tar -cJvf image-456-r456.txz image
`)

		secondImage := openArchive(t, baseDir, "image-456-r456.txz")
		defer func() { _ = secondImage.Close() }()
		require.NoError(t, m.Update(secondImage))

		v, err := m.Version()
		require.NoError(t, err)
		require.Equal(t, "456-r456", v)
	})

	t.Run("doesn't remove old srv if it lays not it /srv-images/", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
mkdir some-strange-image/
ln -s $(pwd)/some-strange-image srv

mkdir image/
echo 'foo: bar' > image/versions.sls
tar -cJvf image-123-r123.txz image
`)
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		image := openArchive(t, baseDir, "image-123-r123.txz")
		defer func() { _ = image.Close() }()
		require.NoError(t, m.Update(image))

		_, err = os.Stat(filepath.Join(baseDir, "some-strange-image"))
		require.NoError(t, err, "some-strange-image/ should exists")
	})

	t.Run("update heals broken /srv (/srv is a directory not a symlink)", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
mkdir srv
echo 'some' > srv/junk

mkdir image/
echo 'foo: bar' > image/versions.sls
tar -cJvf image-123-r123.txz image
`)
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		image := openArchive(t, baseDir, "image-123-r123.txz")
		defer func() { _ = image.Close() }()
		require.NoError(t, m.Update(image))

		v, err := m.Version()
		require.NoError(t, err)
		require.Equal(t, "123-r123", v)
	})

	t.Run("Update don't remove old srv if archive is broken", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
mkdir image/
echo 'foo: bar' > image/versions.sls
tar -cJvf image-123-r545.txz image
echo 'wat' > image-999-r999.txz
`)
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		image := openArchive(t, baseDir, "image-123-r545.txz")
		defer func() { _ = image.Close() }()
		require.NoError(t, m.Update(image))

		brokenImage := openArchive(t, baseDir, "image-999-r999.txz")
		defer func() { _ = brokenImage.Close() }()
		require.Error(t, m.Update(brokenImage))

		v, err := m.Version()
		require.NoError(t, err)
		require.Equal(t, "123-r545", v)
	})

	t.Run("Update to same version is not supported", func(t *testing.T) {
		baseDir := testBaseDir(t)
		cfg := testCfg(baseDir)
		setupWithScript(t, baseDir, `
mkdir image/
echo 'foo: bar' > image/versions.sls
tar -cJvf image-123-r545.txz image
`)
		m, err := srv.New(cfg, &nop.Logger{})
		require.NoError(t, err)

		image := openArchive(t, baseDir, "image-123-r545.txz")
		defer func() { _ = image.Close() }()
		require.NoError(t, m.Update(image))

		sameImage := openArchive(t, baseDir, "image-123-r545.txz")
		defer func() { _ = sameImage.Close() }()
		require.Error(t, m.Update(sameImage))
	})
}
