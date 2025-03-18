package gopackages

import (
	"fmt"
	"os"
	"strings"

	"golang.org/x/tools/go/packages"

	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
)

var _ resolver.Resolver = (*Resolver)(nil)

type Resolver struct {
	dir string
}

func NewResolver(opts ...Option) *Resolver {
	var r Resolver
	for _, opt := range opts {
		opt(&r)
	}
	return &r
}

func (r *Resolver) ResolvePackages(paths ...string) ([]resolver.Package, error) {
	environ := []string{"GOPROXY=off"}
	for _, env := range os.Environ() {
		if strings.HasPrefix(env, "GOPROXY") {
			continue
		}

		environ = append(environ, env)
	}

	pkgs, err := packages.Load(
		&packages.Config{
			Mode: packages.NeedName,
			Dir:  r.dir,
			Env:  environ,
		},
		paths...,
	)

	if err != nil {
		return nil, err
	}

	if len(paths) != len(pkgs) {
		return nil, fmt.Errorf("go returns not all packages: %d (want) < %d (actual)", len(paths), len(pkgs))
	}

	out := make([]resolver.Package, len(pkgs))
	for i, pkg := range pkgs {
		out[i] = resolver.Package{
			Name: pkg.Name,
		}
	}

	return out, nil
}
