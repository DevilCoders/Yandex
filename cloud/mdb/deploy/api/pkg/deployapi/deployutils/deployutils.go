package deployutils

import (
	"context"
	"io/ioutil"
	"os"
	"strconv"
	"strings"

	"github.com/fsnotify/fsnotify"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	deployPath        = "/etc/yandex/mdb-deploy"
	deployAPIHostPath = deployPath + "/mdb_deploy_api_host"
	deployVersionPath = deployPath + "/deploy_version"
	pathPerms         = 0755
)

// DeployAPIURL of this salt-minion.
func DeployAPIURL() (string, error) {
	data, err := ioutil.ReadFile(deployAPIHostPath)
	if err != nil {
		return "", xerrors.Errorf("failed to read mdb deploy api host: %w", err)
	}

	return parseDeployAPIURL(string(data))
}

func parseDeployAPIURL(data string) (string, error) {
	host := strings.TrimSpace(data)
	if host == "" {
		return "", xerrors.New("empty mdb deploy api host file")
	}

	return "https://" + host, nil
}

// DeployVersion of this salt-minion.
func DeployVersion() (int, error) {
	data, err := ioutil.ReadFile(deployVersionPath)
	if err != nil {
		return 0, xerrors.Errorf("failed to read deploy version file: %w", err)
	}

	return parseDeployVersion(string(data))
}

func parseDeployVersion(data string) (int, error) {
	v, err := strconv.ParseInt(strings.TrimSpace(data), 10, 64)
	if err != nil {
		return 0, xerrors.Errorf("failed to parse deploy version file: %w", err)
	}

	return int(v), nil
}

// Info about deploy version
type Info struct {
	Version int
	URL     string
}

// Watcher for deploy version changes
type Watcher struct {
	lg        log.Logger
	fswatcher *fsnotify.Watcher
	ch        chan Info
}

// NewWatcher returns deploy version changes watcher
func NewWatcher(ctx context.Context, lg log.Logger) (*Watcher, error) {
	if err := os.MkdirAll(deployPath, pathPerms); err != nil {
		return nil, xerrors.Errorf("failed to create %q directories: %w", deployPath, err)
	}

	fswatcher, err := fsnotify.NewWatcher()
	if err != nil {
		return nil, xerrors.Errorf("failed to create fsnotify watcher: %w", err)
	}

	if err := fswatcher.Add(deployPath); err != nil {
		_ = fswatcher.Close()
		return nil, xerrors.Errorf("failed to add path %q to watcher: %s", deployPath, err)
	}

	ch := make(chan Info)
	w := &Watcher{lg: lg, fswatcher: fswatcher, ch: ch}
	go w.handleNotify(ctx)
	return w, nil
}

func (w *Watcher) handleNotify(ctx context.Context) {
	defer func() { _ = w.fswatcher.Close() }()
	defer close(w.ch)

	w.sendInfo()

	for {
		select {
		case <-ctx.Done():
			w.lg.Debug("Stopping deploy watcher notify handler...")
			return
		case event, ok := <-w.fswatcher.Events:
			if !ok {
				w.lg.Debug("Deploy watcher notify channel closed.")
				return
			}

			w.lg.Debugf("Deploy watcher notify event: %s", event)

			if event.Op&fsnotify.Create != fsnotify.Create && event.Op&fsnotify.Write != fsnotify.Write {
				continue
			}

			w.sendInfo()
		case err, ok := <-w.fswatcher.Errors:
			if !ok {
				w.lg.Debug("Deploy watcher errors channel closed.")
				return
			}

			w.lg.Errorf("Deploy watcher notify error: %s", err)
		}
	}
}

func (w *Watcher) sendInfo() {
	v, err := DeployVersion()
	if err != nil {
		w.lg.Warnf("Error reading deploy version: %s", err)
		return
	}

	url, err := DeployAPIURL()
	if err != nil {
		w.lg.Warnf("Error reading deploy url: %s", err)
		return
	}

	w.ch <- Info{Version: v, URL: url}
}

// Info returns channel that will report deploy version changes. When watcher is started, current deploy version
// is returned immediately
func (w *Watcher) Info() <-chan Info {
	return w.ch
}
