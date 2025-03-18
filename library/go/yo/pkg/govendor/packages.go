package govendor

import (
	"sort"
	"strings"

	"a.yandex-team.ru/library/go/yo/pkg/fileutil"
	"a.yandex-team.ru/library/go/yo/pkg/gomod"
)

var goEnvironments = [][]string{
	{"GOOS=linux"},
	{"GOOS=windows"},
	{"GOOS=darwin"},
}

func unique(in []string) []string {
	sort.Strings(in)

	var out []string
	for _, s := range in {
		if len(out) == 0 || out[len(out)-1] != s {
			out = append(out, s)
		}
	}

	return out
}

func (p PkgImports) Add(pkg gomod.Package) PkgImports {
	return PkgImports{
		Imports:     unique(append(p.Imports, pkg.Imports...)),
		TestImports: unique(append(append(p.TestImports, pkg.TestImports...), pkg.XTestImports...)),
	}
}

func addTestdataPackages(mod *gomod.MainModule, modulePath string, pkgs map[string]PkgImports, env []string) {
	for {
		foundNew := false

		visitImport := func(importPath string) {
			if _, ok := pkgs[importPath]; ok {
				return
			}

			if !strings.HasPrefix(importPath, modulePath) {
				return
			}

			if !fileutil.IsTestdata(importPath) {
				return
			}

			testdataPkg, _ := mod.ListPackages(importPath, env)
			if len(pkgs) != 0 {
				foundNew = true
				pkgs[importPath] = pkgs[importPath].Add(testdataPkg[0])
			}
		}

		for _, pkg := range pkgs {
			for _, i := range pkg.Imports {
				visitImport(i)
			}

			for _, i := range pkg.TestImports {
				visitImport(i)
			}
		}

		if !foundNew {
			return
		}
	}
}

// GetImports returns module import graph, unified for all supported GOOS values.
func GetImports(mod *gomod.MainModule, modulePath string) (map[string]PkgImports, error) {
	pkgs := map[string]PkgImports{}

	for _, env := range goEnvironments {
		envPkgs, err := mod.ListPackages(modulePath+"/...", env)
		if err != nil {
			return nil, err
		}

		for _, pkg := range envPkgs {
			if pkg.Module.Path != modulePath {
				continue
			}

			pkgs[pkg.ImportPath] = pkgs[pkg.ImportPath].Add(pkg)
		}

		addTestdataPackages(mod, modulePath, pkgs, env)
	}

	return pkgs, nil
}
