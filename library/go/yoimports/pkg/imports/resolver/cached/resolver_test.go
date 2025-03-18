package cached_test

import (
	"errors"
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver/cached"
)

type fakeResolver struct {
	pkgs map[string]resolver.Package
	err  bool
}

func (r *fakeResolver) ResolvePackages(paths ...string) ([]resolver.Package, error) {
	if r.err {
		return nil, errors.New("err resolve")
	}

	out := make([]resolver.Package, len(paths))
	for i, path := range paths {
		pkg, ok := r.pkgs[path]
		if !ok {
			return nil, fmt.Errorf("no pkg: %s", path)
		}

		out[i] = pkg
	}

	return out, nil
}

func TestResolver_store(t *testing.T) {
	cases := []struct {
		path string
		ok   bool
		pkg  resolver.Package
	}{
		{
			path: "lol",
			ok:   true,
			pkg: resolver.Package{
				Name: "lol",
			},
		},
		{
			path: "kek",
			ok:   true,
			pkg: resolver.Package{
				Name: "kek",
			},
		},
		{
			path: "cheburek",
			ok:   false,
		},
	}

	r := fakeResolver{
		pkgs: make(map[string]resolver.Package),
	}
	for _, tc := range cases {
		if !tc.ok {
			continue
		}

		r.pkgs[tc.path] = tc.pkg
	}

	c := cached.NewResolver(&r)
	for _, tc := range cases {
		t.Run("store_"+tc.path, func(t *testing.T) {
			pkgs, err := c.ResolvePackages(tc.path)
			if !tc.ok {
				require.Error(t, err)
				return
			}

			require.NoError(t, err)
			require.Len(t, pkgs, 1)
			require.Equal(t, tc.pkg, pkgs[0])
		})
	}

	r.err = true
	for _, tc := range cases {
		t.Run("load_"+tc.path, func(t *testing.T) {
			pkgs, err := c.ResolvePackages(tc.path)
			if !tc.ok {
				require.Error(t, err)
				return
			}

			require.NoError(t, err)
			require.Len(t, pkgs, 1)
			require.Equal(t, tc.pkg, pkgs[0])
		})
	}
}
