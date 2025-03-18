package fileutil

import (
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"a.yandex-team.ru/library/go/slices"
)

func CopyFile(from, to string) error {
	src, err := os.Open(from)
	if err != nil {
		return err
	}
	defer func() { _ = src.Close() }()

	dst, err := os.Create(to)
	if err != nil {
		return err
	}
	defer func() { _ = dst.Close() }()
	_, err = io.Copy(dst, src)
	return err
}

func CopyTree(from, to string) error {
	return filepath.Walk(from, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if path != from {
			path = path[len(from)+1:]
		} else {
			path = ""
		}

		if info.IsDir() {
			err := os.Mkdir(filepath.Join(to, path), 0755)
			if err != nil && !os.IsExist(err) {
				return err
			}
		} else if info.Mode()&os.ModeSymlink == 0 {
			return CopyFile(filepath.Join(from, path), filepath.Join(to, path))
		}

		return nil
	})
}

func IsTestdata(path string) bool {
	return strings.Contains(path, "/testdata") || strings.Contains(path, "/.") || strings.Contains(path, "/_")
}

func IsDirExists(path string) bool {
	fi, err := os.Stat(path)
	return err == nil && fi.IsDir()
}

// CopyTreeIgnore copies module files, respecting ignore directives.
func CopyTreeIgnore(from, toRoot, toDir string, ignore func(path string, isDir bool) bool) error {
	if err := os.MkdirAll(filepath.Join(toRoot, toDir), 0777); err != nil {
		return err
	}

	return filepath.Walk(from, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if path != from {
			path = path[len(from)+1:]
		} else {
			path = ""
		}

		toPath := filepath.ToSlash(filepath.Join(toDir, path))
		if ignore(toPath, info.IsDir()) {
			if info.IsDir() {
				return filepath.SkipDir
			} else {
				return nil
			}
		}

		if !info.IsDir() && info.Mode()&os.ModeSymlink == 0 {
			dir := filepath.Dir(filepath.Join(toRoot, toPath))
			if err := os.MkdirAll(dir, 0777); err != nil {
				return err
			}

			return CopyFile(filepath.Join(from, path), filepath.Join(toRoot, toPath))
		}

		return nil
	})
}

func CopyGoFiles(fromDir, fromModule, toDir string, pkgs []string, skipTests bool) error {
	for _, pkg := range pkgs {
		if !strings.HasPrefix(pkg, fromModule) {
			return fmt.Errorf("package %s is not part of the module %s", fromModule, pkg)
		}

		relPkg := strings.TrimPrefix(strings.TrimPrefix(pkg, fromModule), "/")
		files, err := ioutil.ReadDir(filepath.Join(fromDir, relPkg))
		if err != nil {
			return err
		}

		if err := os.MkdirAll(filepath.Join(toDir, relPkg), 0777); err != nil {
			return err
		}

		for _, file := range files {
			if file.IsDir() {
				continue
			}

			if filepath.Ext(file.Name()) != ".go" {
				continue
			}

			if skipTests && strings.HasSuffix(file.Name(), "_test.go") {
				continue
			}

			fromFile := filepath.Join(fromDir, relPkg, file.Name())
			toFile := filepath.Join(toDir, relPkg, file.Name())
			if err := CopyFile(fromFile, toFile); err != nil {
				return err
			}
		}
	}

	return nil
}

// RemoveModule removes all module files, skipping other major module versions.
func RemoveModule(vendorRoot, modulePath string, nested []string) error {
	files, err := ioutil.ReadDir(filepath.Join(vendorRoot, modulePath))
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}

		return err
	}

	for _, f := range files {
		path := filepath.Join(modulePath, f.Name())
		if slices.ContainsString(nested, path) {
			continue
		}

		if f.IsDir() {
			if err := RemoveModule(vendorRoot, path, nested); err != nil {
				return err
			}
		} else {
			if err := os.RemoveAll(filepath.Join(vendorRoot, path)); err != nil {
				return err
			}
		}
	}

	return nil
}
