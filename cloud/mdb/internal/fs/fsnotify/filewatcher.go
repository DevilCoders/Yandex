package fsnotify

import (
	"context"
	"os"
	"path/filepath"
	"sync/atomic"

	"github.com/fsnotify/fsnotify"

	"a.yandex-team.ru/cloud/mdb/internal/fs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type FileWatcher struct {
	filePath string
	exists   int32
	ctx      context.Context
	l        log.Logger
	watcher  *fsnotify.Watcher
}

// NewFileWatcher constructs FileWatcher
func NewFileWatcher(ctx context.Context, filePath string, l log.Logger) (*FileWatcher, error) {
	fileName := filepath.Base(filePath)
	fileDir := filepath.Dir(filePath)

	switch _, err := os.Stat(fileDir); {
	case err == nil:
	case os.IsNotExist(err):
		if err := os.MkdirAll(fileDir, fs.DefaultDirPerm); err != nil {
			return nil, xerrors.Errorf("can not create dir %s with error: %w", fileDir, err)
		}
	default:
		return nil, xerrors.Errorf("can not stat given file dir %s with error: %w", fileDir, err)
	}

	realDir, err := filepath.EvalSymlinks(fileDir)
	if err != nil {
		return nil, xerrors.Errorf("can not resolve symlinks dir %s with error: %w", fileDir, err)
	}

	realFilePath := filepath.Join(realDir, fileName)
	if filePath != realFilePath {
		ctxlog.Debugf(ctx, l, "file watcher name resolved %s -> %s", filePath, realFilePath)
	}

	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		return nil, xerrors.Errorf("can not create file watcher: %w", err)
	}

	if err := watcher.Add(realDir); err != nil {
		_ = watcher.Close()
		return nil, xerrors.Errorf("can not add file to file watcher: %w", err)
	}

	fw := &FileWatcher{realFilePath, 1, ctx, l, watcher}

	switch _, err := os.Stat(fw.filePath); {
	case err == nil:
	case os.IsNotExist(err):
		fw.setNotExists()
	default:
		_ = watcher.Close()
		return nil, xerrors.Errorf("failed to stat file %s: %w", fw.filePath, err)
	}

	go func(fw *FileWatcher) {
		defer func() {
			if err := fw.watcher.Close(); err != nil {
				ctxlog.Errorf(ctx, fw.l, "file watcher can not close watcher events channel: %+v", err)
			}
		}()

		for {
			select {
			case event, ok := <-fw.watcher.Events:
				if !ok {
					ctxlog.Debug(ctx, fw.l, "file watcher events channel closed")
				}

				if filepath.Clean(event.Name) == fw.filePath {
					if event.Op&fsnotify.Create == fsnotify.Create {
						ctxlog.Debugf(ctx, fw.l, "file watcher file was created: %s", fw.filePath)
						fw.setExists()
					} else if event.Op&fsnotify.Remove == fsnotify.Remove ||
						event.Op&fsnotify.Rename == fsnotify.Rename {
						ctxlog.Errorf(ctx, fw.l, "file watcher file was removed: %s", fw.filePath)
						fw.setNotExists()
					}
				}

			case err, ok := <-fw.watcher.Errors:
				if !ok {
					ctxlog.Debug(ctx, fw.l, "file watcher errors channel closed")
					return
				}
				ctxlog.Errorf(ctx, fw.l, "file watcher errors chan: %+v", err)

			case <-fw.ctx.Done():
				ctxlog.Debug(ctx, fw.l, "stopping file watcher")
				return
			}
		}
	}(fw)
	return fw, nil
}

// Exists returns if file exists or not
func (fw *FileWatcher) Exists() bool {
	return atomic.LoadInt32(&fw.exists) == 1
}

func (fw *FileWatcher) setExists() {
	atomic.StoreInt32(&fw.exists, 1)
}

func (fw *FileWatcher) setNotExists() {
	atomic.StoreInt32(&fw.exists, 0)
}
