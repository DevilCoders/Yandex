package adaptive

import (
	"go/build"
	"io/ioutil"
	"os"

	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
)

type goPathResolver struct {
	bctx build.Context
}

func newGopathResolver(dir string) *goPathResolver {
	ctx := build.Default
	ctx.GOPATH = envOr("GOPATH", ctx.GOPATH)
	ctx.Dir = dir

	// HACK: setting any of the Context I/O hooks prevents Import from invoking
	// 'go list', regardless of GO111MODULE. This is undocumented, but it's
	// unlikely to change before GOPATH support is removed.
	ctx.ReadDir = ioutil.ReadDir
	return &goPathResolver{
		bctx: ctx,
	}
}

func (r *goPathResolver) ResolvePackages(paths ...string) ([]resolver.Package, error) {
	out := make([]resolver.Package, len(paths))
	for i, path := range paths {
		if build.IsLocalImport(path) {
			// unsupported
			continue
		}

		buildPkg, err := r.bctx.Import(path, r.bctx.Dir, build.FindOnly)
		if err != nil {
			continue
		}

		pkgName, err := packageDirToName(buildPkg.Dir, path, false)
		if err != nil {
			continue
		}

		out[i] = resolver.Package{
			Name: pkgName,
		}
	}

	return out, nil
}

func envOr(name, def string) string {
	s := os.Getenv(name)
	if s == "" {
		return def
	}
	return s
}
