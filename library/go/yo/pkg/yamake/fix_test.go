package yamake

import (
	"os"
	"runtime"
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yo/pkg/testutil"
)

func withModules(t *testing.T) {
	Context.GOPATH = ""

	Context.GOROOT = os.Getenv("GOROOT")
	if Context.GOROOT == "" {
		Context.GOROOT = runtime.GOROOT()
	}

	require.NoError(t, os.Setenv("GO111MODULE", "on"))
}

func withoutModules(t *testing.T) {
	Context.GOPATH = testutil.GOPATH

	Context.GOROOT = os.Getenv("GOROOT")
	if Context.GOROOT == "" {
		Context.GOROOT = runtime.GOROOT()
	}

	require.NoError(t, os.Setenv("GO111MODULE", "off"))
}

func TestGoList(t *testing.T) {
	withoutModules(t)

	importPaths, err := GoList("./vendor/github.com/...", testutil.ArcadiaRoot)
	require.NoError(t, err)

	sort.Strings(importPaths)
	expected := []string{
		"a.yandex-team.ru/vendor/github.com/arch",
		"a.yandex-team.ru/vendor/github.com/bar",
		"a.yandex-team.ru/vendor/github.com/foo",
		"a.yandex-team.ru/vendor/github.com/testonly",
		"a.yandex-team.ru/vendor/github.com/zog",
	}
	assert.Equal(t, expected, importPaths)
}

func TestGoList_Modules(t *testing.T) {
	withModules(t)

	importPaths, err := GoList("github.com/...", testutil.ArcadiaRoot)
	require.NoError(t, err)

	sort.Strings(importPaths)
	expected := []string{
		"github.com/arch",
		"github.com/bar",
		"github.com/foo",
		"github.com/testonly",
		"github.com/zog",
	}
	assert.Equal(t, expected, importPaths)
}

func TestFindSources(t *testing.T) {
	withoutModules(t)

	pkgs, err := FindSources(
		[]string{
			"a.yandex-team.ru/vendor/github.com/foo",
			"a.yandex-team.ru/vendor/github.com/bar",
		},
		testutil.ArcadiaRoot,
	)
	require.NoError(t, err)

	assert.Contains(t, pkgs, "a.yandex-team.ru/vendor/github.com/foo")
	assert.Contains(t, pkgs, "a.yandex-team.ru/vendor/github.com/bar")
}

func TestFindSources_Modules(t *testing.T) {
	withModules(t)

	pkgs, err := FindSources(
		[]string{
			"github.com/foo",
			"github.com/bar",
		},
		testutil.ArcadiaRoot,
	)
	require.NoError(t, err)

	assert.Contains(t, pkgs, "github.com/foo")
	assert.Contains(t, pkgs, "github.com/bar")
}

func TestSyncSrcs(t *testing.T) {
	withoutModules(t)

	list, err := GoList("./vendor/...", testutil.ArcadiaRoot)
	require.NoError(t, err)

	yaMakes, err := SyncSrcs(
		list,
		testutil.ArcadiaRoot,
		nil,
	)
	require.NoError(t, err)

	arch := yaMakes["vendor/github.com/arch"]
	assert.Equal(t, []string{"other.go"}, arch.CommonSources.Files)
	assert.Equal(t, []string{"arch_linux.go"}, arch.TargetSources[Linux].Files)
	assert.Equal(t, []string{"arm64.go"}, arch.TargetSources[LinuxARM64].Files)
	assert.Equal(t, []string{"arch_darwin.go"}, arch.TargetSources[Darwin].Files)
	assert.Equal(t, []string{"amd64_darwin.go"}, arch.TargetSources[DarwinAMD64].Files)
	assert.Equal(t, []string{"arm64.go", "arm64_darwin.go"}, arch.TargetSources[DarwinARM64].Files)
	assert.Equal(t, []string{"arch_windows.go"}, arch.TargetSources[Windows].Files)
	assert.Equal(t, []string{"notarm.go"}, arch.TargetSources[AMD64].Files)

	foo := yaMakes["vendor/github.com/foo"]
	assert.Equal(t, []string{"foo.go"}, foo.CommonSources.Files)
	assert.Equal(t, []string{"foo_test.go"}, foo.CommonSources.TestGoFiles)

	fooTest := yaMakes["vendor/github.com/foo/gotest"]
	assert.Equal(t, fooTest.Module, Macro{Name: "GO_TEST_FOR", Args: []string{"vendor/github.com/foo"}})

	testonly := yaMakes["vendor/github.com/testonly"]
	assert.Equal(t, []string{"foo_test.go"}, testonly.CommonSources.TestGoFiles)
	assert.Equal(t, "GO_TEST", testonly.Module.Name)

	_, ok := yaMakes["vendor/github.com/testonly/gotest"]
	assert.False(t, ok)
}

func TestSyncSrcs_Modules(t *testing.T) {
	withModules(t)

	list, err := GoList("./vendor/github.com/...", testutil.ArcadiaRoot)
	require.NoError(t, err)

	yaMakes, err := SyncSrcs(
		list,
		testutil.ArcadiaRoot,
		nil,
	)
	require.NoError(t, err)

	arch := yaMakes["vendor/github.com/arch"]
	assert.Equal(t, []string{"other.go"}, arch.CommonSources.Files)
	assert.Equal(t, []string{"arch_linux.go"}, arch.TargetSources[Linux].Files)
	assert.Equal(t, []string{"arm64.go"}, arch.TargetSources[LinuxARM64].Files)
	assert.Equal(t, []string{"arch_darwin.go"}, arch.TargetSources[Darwin].Files)
	assert.Equal(t, []string{"amd64_darwin.go"}, arch.TargetSources[DarwinAMD64].Files)
	assert.Equal(t, []string{"arm64.go", "arm64_darwin.go"}, arch.TargetSources[DarwinARM64].Files)
	assert.Equal(t, []string{"arch_windows.go"}, arch.TargetSources[Windows].Files)
	assert.Equal(t, []string{"notarm.go"}, arch.TargetSources[AMD64].Files)

	foo := yaMakes["vendor/github.com/foo"]
	assert.Equal(t, []string{"foo.go"}, foo.CommonSources.Files)
	assert.Equal(t, []string{"foo_test.go"}, foo.CommonSources.TestGoFiles)

	fooTest := yaMakes["vendor/github.com/foo/gotest"]
	assert.Equal(t, fooTest.Module, Macro{Name: "GO_TEST_FOR", Args: []string{"vendor/github.com/foo"}})

	testonly := yaMakes["vendor/github.com/testonly"]
	assert.Equal(t, []string{"foo_test.go"}, testonly.CommonSources.TestGoFiles)
	assert.Equal(t, "GO_TEST", testonly.Module.Name)

	_, ok := yaMakes["vendor/github.com/testonly/gotest"]
	assert.False(t, ok)
}

func TestDisabledRecurseCheck(t *testing.T) {
	for _, testCase := range []struct {
		line, recurse string
		ans           bool
	}{
		{"#broken", "broken", true},
		{"# broken", "broken", true},
		{"# broken ", "broken", true},
		{"# broken TODO", "broken", true},
		{"# broken/foo", "broken", false},
	} {
		assert.Equalf(t, testCase.ans, isDisabledRecurseFor(testCase.line, testCase.recurse), "%#v", testCase)
	}
}

func TestUpdateRecurse(t *testing.T) {
	yaMakes := map[string]*YaMake{
		"vendor":                              {Recurse: []string{"github.com/golang/x/net", "# broken", "# verybroken TODO"}},
		"vendor/broken":                       {},
		"vendor/verybroken":                   {},
		"vendor/github.com/golang/x/net":      {},
		"vendor/github.com/foo/bar":           {TargetRecurse: map[*Target][]string{}},
		"vendor/github.com/foo/bar/zog":       {},
		"vendor/github.com/foo/bar/unix":      {TargetSources: map[*Target]*Sources{Linux: {Files: []string{"a.go"}}}},
		"vendor/go.lang/targetrecurse/linux":  {},
		"vendor/go.lang/targetrecurse/custom": {},
		"vendor/go.lang/targetrecurse/base":   {},
		"vendor/go.lang/targetrecurse": {
			Recurse: []string{
				"base",
				"old",
			},
			TargetRecurse: map[*Target][]string{Linux: {"linux"}},
			CustomConditionRecurse: map[string]struct{}{
				"custom": {},
				"linux":  {},
			},
		},
	}

	UpdateRecurse(yaMakes, ".", "vendor", func(string) *YaMake {
		return NewYaMake()
	})

	assert.Equal(t, []string{"# broken", "github.com", "go.lang", "# verybroken TODO"}, yaMakes["vendor"].Recurse)

	assert.Equal(t, []string{"foo", "golang"}, yaMakes["vendor/github.com"].Recurse)
	assert.Equal(t, []string{"x"}, yaMakes["vendor/github.com/golang"].Recurse)

	assert.Equal(t, []string{"zog"}, yaMakes["vendor/github.com/foo/bar"].Recurse)
	assert.Equal(t, []string{"unix"}, yaMakes["vendor/github.com/foo/bar"].TargetRecurse[Linux])
	assert.Equal(t, []string{"base"}, yaMakes["vendor/go.lang/targetrecurse"].Recurse)
}
