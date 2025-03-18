package main

import (
	"fmt"
	"io"
	"os"
	"path/filepath"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/library/go/test/recipe"
	"a.yandex-team.ru/library/go/test/yatest"
)

func copyFile(from, to string) error {
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

func copyTree(from, to string) error {
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
			return os.MkdirAll(filepath.Join(to, path), 0755)
		} else if info.Mode()&os.ModeSymlink == 0 {
			return copyFile(filepath.Join(from, path), filepath.Join(to, path))
		}

		return nil
	})
}

var (
	testdataDir = pflag.String("testdata-dir", "", "")
)

type testdata struct{}

func (r *testdata) Start() error {
	pflag.CommandLine.ParseErrorsWhitelist.UnknownFlags = true
	pflag.Parse()

	if *testdataDir == "" {
		return fmt.Errorf("--testdata-dir argument is required")
	}

	srcDir := yatest.SourcePath(*testdataDir)

	for {
		st, err := os.Lstat(srcDir)
		if err != nil {
			return err
		}

		if st.Mode()&os.ModeSymlink == 0 {
			break
		}

		srcDir, err = os.Readlink(srcDir)
		if err != nil {
			return err
		}
	}

	if err := copyTree(srcDir, "."); err != nil {
		return err
	}

	return nil
}

func (r *testdata) Stop() error {
	return nil
}

func main() {
	recipe.Run(&testdata{})
}
