package fs

import (
	"io"

	"github.com/fsnotify/fsnotify"
)

//go:generate ../../scripts/mockgen.sh FileWatcher,Watcher

// Watcher is an interface for fsnotify.Watcher wrapper that can be mocked
type Watcher interface {
	io.Closer

	Add(name string) error
	Remove(name string) error
	Events() <-chan fsnotify.Event
	Errors() <-chan error
}
