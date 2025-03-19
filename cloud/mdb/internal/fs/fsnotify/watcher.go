package fsnotify

import (
	"github.com/fsnotify/fsnotify"

	"a.yandex-team.ru/cloud/mdb/internal/fs"
)

type wrapper struct {
	w *fsnotify.Watcher
}

func NewWatcher() (fs.Watcher, error) {
	w, err := fsnotify.NewWatcher()
	if err != nil {
		return nil, err
	}

	return &wrapper{w: w}, nil
}

func (wr *wrapper) Add(name string) error {
	return wr.w.Add(name)
}

func (wr *wrapper) Close() error {
	return wr.w.Close()
}

func (wr *wrapper) Remove(name string) error {
	return wr.w.Remove(name)
}

func (wr *wrapper) Events() <-chan fsnotify.Event {
	return wr.w.Events
}

func (wr *wrapper) Errors() <-chan error {
	return wr.w.Errors
}
