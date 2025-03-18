package gomod

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"reflect"
	"sort"
	"strings"

	"a.yandex-team.ru/library/go/yo/pkg/fileutil"
)

type MainModule struct {
	ArcadiaRoot string
	Path        string

	goMod, goSum string
}

func CreateFakeModule(arcadiaRoot string) (*MainModule, error) {
	dir, err := ioutil.TempDir("", "fake_module")
	if err != nil {
		return nil, fmt.Errorf("failed to create tmp dir: %v", err)
	}

	m := &MainModule{ArcadiaRoot: arcadiaRoot, Path: dir}

	m.goMod = filepath.Join("vendor", "ya.mod")
	err = fileutil.CopyFile(filepath.Join(arcadiaRoot, m.goMod), filepath.Join(dir, "go.mod"))
	if os.IsNotExist(err) {
		m.goMod = "go.mod"
		err = fileutil.CopyFile(filepath.Join(arcadiaRoot, m.goMod), filepath.Join(dir, "go.mod"))
	}

	if err != nil {
		_ = os.RemoveAll(dir)
		return nil, fmt.Errorf("failed to copy ya.mod: %v", err)
	}

	m.goSum = filepath.Join("vendor", "ya.sum")
	err = fileutil.CopyFile(filepath.Join(arcadiaRoot, m.goSum), filepath.Join(dir, "go.sum"))
	if os.IsNotExist(err) {
		m.goSum = "go.sum"
		err = fileutil.CopyFile(filepath.Join(arcadiaRoot, m.goSum), filepath.Join(dir, "go.sum"))
	}

	if err != nil {
		_ = os.RemoveAll(dir)
		return nil, fmt.Errorf("failed to copy ya.sum: %v", err)
	}

	return m, nil
}

func (m *MainModule) Remove() error {
	return os.RemoveAll(m.Path)
}

func (m *MainModule) GoGet(pkg string) error {
	cmd := exec.Command(GoBinary(), "get", "-d", pkg)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Dir = m.Path
	err := cmd.Run()
	if err != nil {
		return fmt.Errorf("go get failed: %v", err)
	}
	return nil
}

func (m *MainModule) queryJSON(args []string, env []string, list interface{}) error {
	cmd := exec.Command(GoBinary(), args...)
	var out bytes.Buffer

	cmd.Stdout = &out
	cmd.Stderr = os.Stderr
	cmd.Dir = m.Path
	cmd.Env = append(os.Environ(), env...)

	if err := cmd.Run(); err != nil {
		return fmt.Errorf("go %s: %v", strings.Join(args, " "), err)
	}

	d := json.NewDecoder(&out)
	slice := reflect.ValueOf(list).Elem()

	for {
		item := reflect.New(reflect.TypeOf(list).Elem().Elem())

		err := d.Decode(item.Interface())
		if err != nil && err != io.EOF {
			return fmt.Errorf("can't parse go output: %v", err)
		} else if err == io.EOF {
			break
		}

		slice.Set(reflect.Append(slice, item.Elem()))
	}

	return nil
}

func (m *MainModule) ListPackages(pattern string, env []string) ([]Package, error) {
	var pkgs []Package

	if err := m.queryJSON([]string{"list", "-mod=mod", "-e", "-json", pattern}, env, &pkgs); err != nil {
		return nil, err
	}

	var okPkgs []Package
	for _, pkg := range pkgs {
		// Workaround for https://github.com/golang/go/issues/38678
		if pkg.Incomplete && len(pkg.IgnoredGoFiles) != 0 {
			continue
		}

		if pkg.Error != nil {
			// Workaround for flatbuffers. Just skip test packages mixed of different languages, such as flatbuffers tests.
			if strings.HasPrefix(pkg.Error.Err, "C++ source files not allowed when not using cgo or SWIG") {
				continue
			}

			return nil, fmt.Errorf("%s: %s", pkg.ImportPath, pkg.Error.Err)
		}

		okPkgs = append(okPkgs, pkg)
	}

	return okPkgs, nil
}

func (m *MainModule) ListModuleUpdates() ([]ListModule, error) {
	var modules []ListModule

	if err := m.queryJSON([]string{"list", "-u", "-e", "-m", "-json", "all"}, nil, &modules); err != nil {
		return nil, err
	}

	sort.Slice(modules, func(i, j int) bool {
		return modules[i].Path < modules[j].Path
	})
	return modules, nil
}

func (m *MainModule) DownloadModules() ([]Module, error) {
	var modules []Module

	if err := m.queryJSON([]string{"mod", "download", "-json"}, nil, &modules); err != nil {
		return nil, err
	}

	return modules, nil
}

func (m *MainModule) AppendSum(modules []Module) error {
	goSum, err := os.OpenFile(filepath.Join(m.Path, "go.sum"), os.O_APPEND|os.O_WRONLY, 0)
	if err != nil {
		return err
	}
	defer goSum.Close()

	buf := bufio.NewWriter(goSum)

	for _, dl := range modules {
		_, _ = fmt.Fprintf(buf, "%s %s %s\n", dl.Path, dl.Version, dl.Sum)
	}

	return buf.Flush()
}

func (m *MainModule) IsGoModChanged() (bool, error) {
	original, err := ioutil.ReadFile(filepath.Join(m.ArcadiaRoot, m.goMod))
	if err != nil {
		return false, err
	}

	copy, err := ioutil.ReadFile(filepath.Join(m.Path, "go.mod"))
	if err != nil {
		return false, err
	}

	return !bytes.Equal(original, copy), nil
}

func (m *MainModule) Save() error {
	err := fileutil.CopyFile(filepath.Join(m.Path, "go.sum"), filepath.Join(m.ArcadiaRoot, m.goSum))
	if err != nil {
		return fmt.Errorf("failed to save ya.sum: %v", err)
	}

	err = fileutil.CopyFile(filepath.Join(m.Path, "go.mod"), filepath.Join(m.ArcadiaRoot, m.goMod))
	if err != nil {
		return fmt.Errorf("failed to save ya.mod: %v", err)
	}

	return nil
}
