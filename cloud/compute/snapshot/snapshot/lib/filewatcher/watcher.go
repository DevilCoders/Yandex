package filewatcher

import (
	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"context"
	"github.com/fsnotify/fsnotify"
	"go.uber.org/zap"
	"io/ioutil"
	"sync"
)

type FileWatcher struct {
	mtx     sync.RWMutex
	content string

	stop chan struct{}
}

type Watcher interface {
	GetString() string
	Close()
}

func NewNilWatcher() NilWatcher {
	return NilWatcher{}
}

func NewWatcher(ctx context.Context, file string) (*FileWatcher, error) {
	updater := &FileWatcher{
		mtx:  sync.RWMutex{},
		stop: make(chan struct{}),
	}

	w, err := fsnotify.NewWatcher()
	if err != nil {
		return nil, err
	}

	if err = updater.readFileContent(file); err != nil {
		return nil, err
	}
	if err = w.Add(file); err != nil {
		return nil, err
	}

	go func() {
		for {
			select {
			case <-updater.stop:
				log.G(ctx).Info("filewatcher stopped from Cancel()")
				return
			case event, ok := <-w.Events:
				if !ok {
					log.G(ctx).Info("w.Events closed, returning")
					return
				}
				if event.Op&fsnotify.Write == fsnotify.Write {
					e := updater.readFileContent(event.Name)
					log.InfoErrorCtx(ctx, e, "file is modified, reading new content", zap.String("file", event.Name))
				}
			case err, ok := <-w.Errors:
				if !ok {
					log.G(ctx).Info("w.Errors closed, returning")
				}
				log.DebugErrorCtx(ctx, err, "error from w.Errors")
			}
		}
	}()

	return updater, nil
}

func (u *FileWatcher) GetString() string {
	u.mtx.RLock()
	defer u.mtx.RUnlock()
	return u.content
}

func (u *FileWatcher) Close() {
	close(u.stop)
}

func (u *FileWatcher) readFileContent(file string) error {
	u.mtx.Lock()
	defer u.mtx.Unlock()

	data, err := ioutil.ReadFile(file)
	if err != nil {
		return err
	}

	u.content = string(data)

	return nil
}

type NilWatcher struct{}

func (n NilWatcher) GetString() string {
	return ""
}

func (n NilWatcher) Close() {}
