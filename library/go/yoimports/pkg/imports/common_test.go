package imports

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestImportPathToAssumedName(t *testing.T) {
	cases := []struct {
		path string
		name string
	}{
		{
			path: "fmt",
			name: "fmt",
		},
		{
			path: "net/http",
			name: "http",
		},
		{
			path: "github.com/lol/kek",
			name: "kek",
		},
		{
			path: "github.com/hashicorp/golang-lru",
			name: "golang",
		},
		{
			path: "gopkg.in/v2yaml",
			name: "v2yaml",
		},
		{
			path: "gopkg.in/yaml.v2",
			name: "yaml",
		},
		{
			path: "github.com/go-resty/resty/v2",
			name: "resty",
		},
		{
			path: "github.com/go-resty/go-resty/v2",
			name: "resty",
		},
		{
			path: "github.com/go-resty/go-resty/v2",
			name: "resty",
		},
	}

	for _, tc := range cases {
		t.Run(tc.path, func(t *testing.T) {
			actual := importPathToAssumedName(tc.path)
			require.Equal(t, tc.name, actual)
		})
	}
}
