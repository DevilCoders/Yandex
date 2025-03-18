package adaptive_test

import (
	"os"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver/adaptive"
)

func TestGoPath(t *testing.T) {
	cases := []struct {
		importPath string
		pkgName    string
	}{
		{
			importPath: "gopath.com/cheburek",
			pkgName:    "cheburek",
		},
		{
			importPath: "github.com/buglloc/mega-blah",
			pkgName:    "not_so_mega_blah",
		},
		{
			importPath: "github.com/buglloc/kek",
			pkgName:    "kek",
		},
		{
			importPath: "github.com/something",
			pkgName:    "",
		},
		{
			importPath: "gopath.com/cheburek/cheburek/something",
			pkgName:    "",
		},
	}

	_ = os.Setenv("GOPATH", "testdata")
	r := adaptive.NewResolver(
		adaptive.WithDir("testdata/src/gopath.com"),
		adaptive.WithFallbackAllowed(false),
	)
	for _, tc := range cases {
		t.Run(tc.importPath, func(t *testing.T) {
			pkgs, err := r.ResolvePackages(tc.importPath)
			require.NoError(t, err)
			require.Len(t, pkgs, 1)
			require.Equal(t, tc.pkgName, pkgs[0].Name)
		})
	}
}

func TestGoModules(t *testing.T) {
	cases := []struct {
		importPath string
		pkgName    string
	}{
		{
			importPath: "test.yandex/modules/cheburek",
			pkgName:    "cheburek",
		},
		{
			importPath: "test.yandex/modules/protos/good",
			pkgName:    "good",
		},
		{
			importPath: "test.yandex/modules/protos/proto",
			pkgName:    "bad",
		},
		{
			importPath: "test.yandex/modules/protos/proto_2",
			pkgName:    "bad2",
		},
		{
			importPath: "test.yandex/modules/protos/broken",
			pkgName:    "",
		},
		{
			importPath: "test.yandex/modules/protos/wogo",
			pkgName:    "",
		},
		{
			importPath: "github.com/buglloc/mega-blah",
			pkgName:    "not_so_mega_blah",
		},
		{
			importPath: "github.com/buglloc/kek",
			pkgName:    "kek",
		},
		{
			importPath: "github.com/go-chi/chi",
			pkgName:    "chi",
		},
		{
			importPath: "github.com/something",
			pkgName:    "",
		},
		{
			importPath: "test.yandex/modules/cheburek/something",
			pkgName:    "",
		},
	}

	r := adaptive.NewResolver(
		adaptive.WithDir("testdata/src/modules.org"),
		adaptive.WithFallbackAllowed(false),
	)
	for _, tc := range cases {
		t.Run(tc.importPath, func(t *testing.T) {
			pkgs, err := r.ResolvePackages(tc.importPath)
			require.NoError(t, err)
			require.Len(t, pkgs, 1)
			require.Equal(t, tc.pkgName, pkgs[0].Name)
		})
	}
}

func TestGoModules_nested(t *testing.T) {
	r := adaptive.NewResolver(
		adaptive.WithDir("testdata/src/modules.org/packages/chi"),
		adaptive.WithFallbackAllowed(false),
	)
	pkgs, err := r.ResolvePackages("github.com/go-chi/chi/internal")
	require.Error(t, err)
	require.Len(t, pkgs, 0)
}
