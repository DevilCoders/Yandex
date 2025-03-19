package srv

import (
	"bytes"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	SrvPath    string `json:"srv_path" yaml:"srv_path"`
	ImagesDir  string `json:"images_dir" yaml:"images_dir"`
	TarCommand string `json:"tar_command" yaml:"tar_command"`
}

func DefaultConfig() Config {
	return Config{
		SrvPath:    "/srv",
		ImagesDir:  "/srv-images",
		TarCommand: "tar",
	}
}

type Version struct {
	Version string
	BuildAt time.Time
}

/*
Manager control /srv data

As on salt-masters we save data in /srv-images
and /srv is symbolic link

[PROD]root@deploy-salt01k ~ # readlink /srv
/srv-images/image-1644250258-r9116588
*/
type Manager struct {
	cfg Config
	l   log.Logger
}

var _ agent.SrvManager = &Manager{}

func safePath(p string) error {
	abs, err := filepath.Abs(p)
	if err != nil {
		return xerrors.Errorf("convert to absolute: %w", err)
	}
	abs = filepath.Clean(abs)
	if abs == "/" {
		return xerrors.Errorf("path / is unsafe")
	}
	return nil
}

func versionFromImagePath(p string) string {
	imageDir := path.Base(p)
	return commander.VersionFromFromName(imageDir)
}

func New(cfg Config, l log.Logger) (*Manager, error) {
	if err := safePath(cfg.SrvPath); err != nil {
		return nil, xerrors.Errorf("srv_path is unsafe: %w", err)
	}
	if err := safePath(cfg.ImagesDir); err != nil {
		return nil, xerrors.Errorf("images_dir is unsafe path: %w", err)
	}
	return &Manager{cfg: cfg, l: l}, nil
}

func (m *Manager) Version() (string, error) {
	realPath, err := os.Readlink(m.cfg.SrvPath)
	if err != nil {
		if xerrors.Is(err, os.ErrNotExist) {
			return "", agent.ErrSrvNotInitialized.Wrap(xerrors.New("/srv not exists"))
		}
		return "", agent.ErrSrvMalformed.Wrap(xerrors.Errorf("/srv not a symbolic link: %w", err))
	}
	fi, err := os.Stat(realPath)
	if err != nil {
		return "", agent.ErrSrvMalformed.Wrap(xerrors.Errorf("/srv points to broken path: %w", err))
	}
	if !fi.IsDir() {
		return "", agent.ErrSrvMalformed.Wrap(xerrors.Errorf("/srv points not to a directory: %s", realPath))
	}

	return versionFromImagePath(realPath), nil
}

func (m *Manager) extractImage(imageName string, image io.ReadCloser) error {
	imageAbsPath := path.Join(m.cfg.ImagesDir, imageName)
	if _, imageErr := os.Stat(imageAbsPath); imageErr == nil {
		m.l.Warnf("Looks like that image dir '%s' exist. Better remove it in case of garbage", imageAbsPath)
		if err := os.RemoveAll(imageAbsPath); err != nil {
			return xerrors.Errorf("remove old (probably that image retry) image data %q: %w", imageAbsPath, err)
		}
	}
	if err := os.MkdirAll(imageAbsPath, fs.ModePerm); err != nil {
		return xerrors.Errorf("create image dir: %w", err)
	}

	cmd := exec.Command(m.cfg.TarCommand, "xJf", "-", "--directory="+imageName, "--strip-components=1")
	cmd.Stdin = image
	cmd.Dir = m.cfg.ImagesDir

	stdout := &bytes.Buffer{}
	stderr := &bytes.Buffer{}
	cmd.Stdout = stdout
	cmd.Stderr = stderr
	if err := cmd.Run(); err != nil {
		return xerrors.Errorf("extracting image, command '%s': %s (stdout: %s) (stderr: %s)", cmd, err, stdout.String(), stderr.String())
	}
	return nil
}

func (m *Manager) updateMountLink(imageName string) error {
	imageAbsPath, err := filepath.Abs(filepath.Join(m.cfg.ImagesDir, imageName))
	if err != nil {
		return xerrors.Errorf("build %s image absolute path: %w", imageName, err)
	}
	tmpLink := m.cfg.SrvPath + ".tmp"
	if _, tmpErr := os.Stat(tmpLink); tmpErr == nil {
		m.l.Warn("Temporary link to image already exists. I remove it")
		if err := os.Remove(tmpLink); err != nil {
			return xerrors.Errorf("removing old temporary link %s: %w", tmpLink, err)
		}
	}

	if err := os.Symlink(imageAbsPath, tmpLink); err != nil {
		return xerrors.Errorf("creating symlink to %s image: %w", imageAbsPath, err)
	}

	mountInfo, err := os.Lstat(m.cfg.SrvPath)
	if err != nil {
		m.l.Infof("accessing to /srv: %s", err)
	} else {
		if mountInfo.Mode()&os.ModeSymlink != os.ModeSymlink {
			m.l.Warnf("%s is not a symbolic link. I remove it", m.cfg.SrvPath)
			if err := os.RemoveAll(m.cfg.SrvPath); err != nil {
				return xerrors.Errorf("removing old %s: %w", m.cfg.SrvPath, err)
			}
		}
	}
	if err := os.Rename(tmpLink, m.cfg.SrvPath); err != nil {
		return xerrors.Errorf("renaming %s to %s: %w", tmpLink, m.cfg.SrvPath, err)
	}
	return nil
}

// Update updates /srv from given archive
func (m *Manager) Update(image datasource.NamedReadCloser) error {
	defer func() {
		if closeErr := image.Close(); closeErr != nil {
			m.l.Warnf("error while closing %s image archive: %s", image.Name(), closeErr)
		}
	}()
	oldSrvPath, err := os.Readlink(m.cfg.SrvPath)
	if err != nil {
		m.l.Infof("unable to access to current /srv path: %s (probably it's not initialized yet)", err)
	}
	oldSrvVersion := versionFromImagePath(oldSrvPath)

	imageName := filepath.Base(image.Name())
	imageName = strings.TrimSuffix(imageName, filepath.Ext(imageName))
	if oldSrvVersion == commander.VersionFromFromName(imageName) {
		// In a process of such an update we
		// - cleanup the current /srv
		// - unpack new SRV into it
		// - then remove the old /srv
		//
		// And finished with a broken symlink
		return xerrors.New("updating /srv to the same version is not supported")
	}

	startAt := time.Now()
	if err := m.extractImage(imageName, image); err != nil {
		return err
	}
	m.l.Infof("successfully extract image in %s", time.Since(startAt))

	startAt = time.Now()
	if err := m.updateMountLink(imageName); err != nil {
		return err
	}
	m.l.Infof("successfully update /srv mount link in %s", time.Since(startAt))

	if oldSrvPath != "" {
		// Remove old /srv since we successfully switch it to a new version
		oldSrvParentDir, _ := filepath.Split(oldSrvPath)
		if filepath.Clean(oldSrvParentDir) != filepath.Clean(m.cfg.ImagesDir) {
			m.l.Warnf("old /srv path '%s' doesn't lays in images_dir: '%s', so I don't remove it", oldSrvPath, m.cfg.ImagesDir)
		} else {
			if err := os.RemoveAll(oldSrvPath); err != nil {
				m.l.Warnf("error while purging old /srv path '%s': %s", oldSrvPath, err)
			}
		}
	}

	return nil
}
