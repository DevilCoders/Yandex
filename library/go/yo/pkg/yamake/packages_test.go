package yamake

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yo/pkg/testutil"
)

func TestImport(t *testing.T) {
	withoutModules(t)

	p, err := Import("a", testutil.ArcadiaRoot)
	require.NoError(t, err)

	assert.Equal(t, "a", p.Name)
	assert.Equal(t, []string{"a.go", "a.s"}, p.CommonSources.Files)

	assert.Equal(t, []string{"a_test.go"}, p.CommonSources.TestGoFiles)
	assert.Equal(t, []string{"external_test.go"}, p.CommonSources.XTestGoFiles)

	p, err = Import("b", Context.GOPATH+"/src/a")
	require.NoError(t, err)
	assert.Equal(t, "a/vendor/b", p.ImportPath)
}

func TestImport_Modules(t *testing.T) {
	withModules(t)

	p, err := Import("a.yandex-team.ru/library", testutil.ArcadiaRoot)
	require.NoError(t, err)

	assert.Equal(t, "library", p.Name)
	assert.Equal(t, []string{"a.go"}, p.CommonSources.Files)

	p, err = Import("github.com/foo", testutil.ArcadiaRoot)
	require.NoError(t, err)

	assert.Equal(t, "foo", p.Name)
	assert.Equal(t, []string{"foo.go"}, p.CommonSources.Files)
}

func TestInvalidImport(t *testing.T) {
	withoutModules(t)

	_, err := Import("c", "")
	assert.Error(t, err)
}
