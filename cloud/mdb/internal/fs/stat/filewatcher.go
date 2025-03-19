package stat

import (
	"os"
)

type FileWatcher struct {
	filePath string
}

// NewFileWatcher constructs FileWatcher
func NewFileWatcher(filePath string) (*FileWatcher, error) {
	fw := &FileWatcher{filePath}
	return fw, nil
}

// Exists returns if file exists or not
func (fw *FileWatcher) Exists() bool {
	_, err := os.Stat(fw.filePath)
	return err == nil
}
