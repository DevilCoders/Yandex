package fs

import (
	"os"
)

const (
	// DefaultDirPerm defines filemode for directory created (if needed) by FileWatcher
	DefaultDirPerm os.FileMode = 0755
)

type FileWatcher interface {
	Exists() bool
}
