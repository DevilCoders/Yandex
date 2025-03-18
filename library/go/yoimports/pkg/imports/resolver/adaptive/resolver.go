package adaptive

import (
	"errors"
	"os"
	"path/filepath"
	"sync"

	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver/golist"
)

var _ resolver.Resolver = (*Resolver)(nil)

type Resolver struct {
	dir             string
	fallbackAllowed bool
	initErr         error
	initOnce        sync.Once
	realResolver    resolver.Resolver
}

func NewResolver(opts ...Option) *Resolver {
	r := &Resolver{
		fallbackAllowed: true,
	}

	for _, opt := range opts {
		opt(r)
	}
	return r
}

func (r *Resolver) ResolvePackages(paths ...string) ([]resolver.Package, error) {
	if err := r.init(); err != nil {
		return nil, err
	}

	return r.realResolver.ResolvePackages(paths...)
}

func (r *Resolver) init() error {
	r.initOnce.Do(func() {
		newResolver := func() error {
			if r.dir == "" {
				wd, err := os.Getwd()
				if err != nil {
					return err
				}
				r.dir = wd
			}

			modRoot := findModRoot(r.dir)
			if modRoot == "" {
				r.realResolver = newGopathResolver(r.dir)
				return nil
			}

			fi, err := os.Stat(filepath.Join(modRoot, "vendor"))
			if err != nil || !fi.IsDir() {
				return errors.New("supported only vendored modules")
			}

			r.realResolver, err = newGoModResolver(modRoot)
			return err
		}

		err := newResolver()
		if err == nil {
			return
		}

		if r.fallbackAllowed {
			r.realResolver = golist.NewResolver(
				golist.WithDir(r.dir),
			)
			return
		}

		r.initErr = err
	})

	return r.initErr
}
