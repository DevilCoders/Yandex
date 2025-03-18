package cached

import (
	"sync"

	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
)

var _ resolver.Resolver = (*Resolver)(nil)

type Resolver struct {
	cache       sync.Map
	subResolver resolver.Resolver
}

func NewResolver(subResolver resolver.Resolver) *Resolver {
	return &Resolver{
		subResolver: subResolver,
	}
}

func (r *Resolver) load(pkgPath string) (resolver.Package, bool) {
	pkg, ok := r.cache.Load(pkgPath)
	if !ok {
		return resolver.Package{}, false
	}

	return pkg.(resolver.Package), true
}

func (r *Resolver) store(pkgPath string, pkg resolver.Package) {
	r.cache.Store(pkgPath, pkg)
}

func (r *Resolver) ResolvePackages(paths ...string) ([]resolver.Package, error) {
	out := make([]resolver.Package, len(paths))
	var toResolve []string
	var pkgPos []int
	for i, path := range paths {
		pkg, ok := r.load(path)
		if ok {
			out[i] = pkg
			continue
		}

		toResolve = append(toResolve, path)
		pkgPos = append(pkgPos, i)
	}

	if len(toResolve) == 0 || len(pkgPos) == 0 {
		return out, nil
	}

	resolvedPkgs, err := r.subResolver.ResolvePackages(toResolve...)
	if err != nil {
		return nil, err
	}

	for i, pkg := range resolvedPkgs {
		r.store(paths[i], pkg)
		out[pkgPos[i]] = pkg
	}

	return out, nil
}
