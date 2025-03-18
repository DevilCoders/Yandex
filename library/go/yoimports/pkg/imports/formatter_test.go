package imports

import (
	"strconv"
	"testing"

	"github.com/dave/dst"
	"github.com/stretchr/testify/require"
)

func TestImportGroup(t *testing.T) {
	type subCase struct {
		path  string
		group int
	}

	cases := []struct {
		local string
		cases []subCase
	}{
		{
			local: "",
			cases: []subCase{
				{
					path:  "a.yandex-team.ru",
					group: impGroup3rdParty,
				},
				{
					path:  "a.yandex-team.ru/a/b",
					group: impGroup3rdParty,
				},
				{
					path:  "golang.yandex/hasql",
					group: impGroup3rdParty,
				},
				{
					path:  "golang.yandex/cheburek",
					group: impGroup3rdParty,
				},
				{
					path:  "github.com/lol/kek",
					group: impGroup3rdParty,
				},
				{
					path:  "fmt",
					group: impGroupSTD,
				},
				{
					path:  "net/http",
					group: impGroupSTD,
				},
			},
		},
		{
			local: "a.yandex-team.ru",
			cases: []subCase{
				{
					path:  "a.yandex-team.ru",
					group: impGroupLocal,
				},
				{
					path:  "a.yandex-team.ru/a/b",
					group: impGroupLocal,
				},
				{
					path:  "golang.yandex/hasql",
					group: impGroup3rdParty,
				},
				{
					path:  "golang.yandex/cheburek",
					group: impGroup3rdParty,
				},
				{
					path:  "github.com/lol/kek",
					group: impGroup3rdParty,
				},
				{
					path:  "fmt",
					group: impGroupSTD,
				},
				{
					path:  "net/http",
					group: impGroupSTD,
				},
			},
		},
		{
			local: "golang.yandex/hasql",
			cases: []subCase{
				{
					path:  "a.yandex-team.ru",
					group: impGroup3rdParty,
				},
				{
					path:  "a.yandex-team.ru/a/b",
					group: impGroup3rdParty,
				},
				{
					path:  "golang.yandex/hasql",
					group: impGroupLocal,
				},
				{
					path:  "golang.yandex/hasql/kekker",
					group: impGroupLocal,
				},
				{
					path:  "golang.yandex/cheburek",
					group: impGroup3rdParty,
				},
				{
					path:  "github.com/lol/kek",
					group: impGroup3rdParty,
				},
				{
					path:  "fmt",
					group: impGroupSTD,
				},
				{
					path:  "net/http",
					group: impGroupSTD,
				},
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.local, func(t *testing.T) {
			f := formatter{
				localPrefix: tc.local,
			}

			for _, stc := range tc.cases {
				t.Run(stc.path, func(t *testing.T) {
					spec := &dst.ImportSpec{
						Path: &dst.BasicLit{
							Value: strconv.Quote(stc.path),
						},
					}

					require.Equal(t, stc.group, f.importGroup(spec))
				})
			}
		})
	}
}
