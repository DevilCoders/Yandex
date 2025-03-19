package tools

import (
	"fmt"
	"io"
	"io/fs"
	"strings"
)

// LoadYAMLs uses with fs.Walk to load every subsequent yaml file by ldr
func LoadYAMLs(files fs.FS, ldr LoadFunc) fs.WalkDirFunc {
	return func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		skip := fs.SkipDir
		name := d.Name()
		if !d.IsDir() {
			skip = nil
		}

		switch {
		case strings.HasPrefix(name, "_"):
			return skip
		case strings.HasPrefix(name, "."):
			return skip
		case d.IsDir():
			return nil
		case !(strings.HasSuffix(name, ".yaml") || strings.HasSuffix(name, ".yml")):
			return nil
		}

		f, err := files.Open(path)
		if err != nil {
			return fmt.Errorf("%s:%w", path, err)
		}
		defer func() { _ = f.Close() }()

		if err := ldr(path, f); err != nil {
			return fmt.Errorf("%s:%w", path, err)
		}
		return nil
	}
}

type LoadFunc func(string, io.Reader) error
