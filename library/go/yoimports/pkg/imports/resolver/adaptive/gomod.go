package adaptive

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"golang.org/x/mod/modfile"

	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
)

type goModResolver struct {
	root        string
	vendor      string
	localPrefix string
}

func newGoModResolver(dir string) (*goModResolver, error) {
	modPath := filepath.Join(dir, "go.mod")
	modBytes, err := ioutil.ReadFile(modPath)
	if err != nil {
		return nil, err
	}

	mod, err := modfile.ParseLax(modPath, modBytes, nil)
	if err != nil {
		return nil, err
	}

	if mod.Module == nil {
		return nil, fmt.Errorf("no module in %s", modPath)
	}

	return &goModResolver{
		root:        dir,
		vendor:      filepath.Join(dir, "vendor"),
		localPrefix: strings.TrimRight(mod.Module.Mod.Path, "/") + "/",
	}, nil
}

func (r *goModResolver) ResolvePackages(paths ...string) ([]resolver.Package, error) {
	out := make([]resolver.Package, len(paths))
	for i, importPath := range paths {
		var (
			pkgDir     string
			parseProto bool
		)
		if strings.HasPrefix(importPath, r.localPrefix) {
			pkgDir = filepath.Join(r.root, filepath.FromSlash(importPath[len(r.localPrefix):]))
			// we try to parse .proto files only for local imports
			parseProto = true
		} else {
			pkgDir = filepath.Join(r.vendor, filepath.FromSlash(importPath))
		}

		pkgName, err := packageDirToName(pkgDir, importPath, parseProto)
		if err != nil {
			continue
		}

		out[i] = resolver.Package{
			Name: pkgName,
		}
	}

	return out, nil
}

func findModRoot(dir string) string {
	if env := os.Getenv("GO111MODULE"); env == "off" {
		return ""
	}

	if dir == "" {
		panic("dir not set")
	}

	dir = filepath.Clean(dir)
	// Look for enclosing go.mod.
	for {
		if fi, err := os.Stat(filepath.Join(dir, "go.mod")); err == nil && !fi.IsDir() {
			return dir
		}
		d := filepath.Dir(dir)
		if d == dir {
			break
		}
		dir = d
	}

	return ""
}
