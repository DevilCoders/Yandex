package govendor

import (
	"bytes"
	"errors"
	"io/ioutil"
	"os"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/test/go_toolchain/gotoolchain"
	"a.yandex-team.ru/library/go/yo/pkg/fileutil"
	"a.yandex-team.ru/library/go/yo/pkg/gomod"
	"a.yandex-team.ru/library/go/yo/pkg/testutil"
	"a.yandex-team.ru/library/go/yo/pkg/yamake"
)

const vendorPolicy = `
ALLOW vendor/ -> .*

ALLOW .* -> vendor/golang.org/x/sys
ALLOW .* -> vendor/rsc.io/quote

DENY .* -> vendor/
`

func setupArcadia(t *testing.T) string {
	require.NoError(t, gotoolchain.Setup(os.Setenv))
	yamake.Context.GOROOT = os.Getenv("GOROOT")

	testutil.NeedProxyAccess(t)

	tmpGOPATH, err := ioutil.TempDir("", "")
	require.NoError(t, err)

	t.Logf("copying GOPATH from %s", testutil.GOPATH)
	require.NoError(t, fileutil.CopyTree(testutil.GOPATH, tmpGOPATH))

	yamake.Context.GOPATH = tmpGOPATH
	tmpArcadiaRoot := filepath.Join(tmpGOPATH, "src", "a.yandex-team.ru")

	t.Logf("running inside %s", tmpArcadiaRoot)
	return tmpArcadiaRoot
}

func TestInitialVendor(t *testing.T) {
	tmpArcadiaRoot := setupArcadia(t)

retry:
	m, err := gomod.CreateFakeModule(tmpArcadiaRoot)
	require.NoError(t, err)

	t.Logf("fake module is located in %s", m.Path)

	require.NoError(t, m.GoGet("rsc.io/quote"))
	require.NoError(t, m.GoGet("rsc.io/quote/v3"))
	require.NoError(t, m.GoGet("golang.org/x/sys"))
	require.NoError(t, m.Save())

	rules, err := yamake.ParsePolicy(bytes.NewBufferString(vendorPolicy))
	require.NoError(t, err)

	vendor, err := NewVendor(tmpArcadiaRoot, m)
	require.NoError(t, err)

	plan, err := vendor.Plan(rules)
	if errors.Is(err, ErrGoModChanged) {
		require.NoError(t, m.Save())
		goto retry
	}

	require.NoError(t, err)

	require.Contains(t, plan, "rsc.io/quote")
	require.Contains(t, plan, "rsc.io/quote/v3")
	require.Contains(t, plan, "golang.org/x/sys")

	for name, m := range plan {
		t.Logf("vendoring %s", name)
		require.NoError(t, vendor.VendorModule(m))
	}

	require.NoError(t, vendor.SaveModulesTXT())

	vendorReload, err := NewVendor(tmpArcadiaRoot, m)
	require.NoError(t, err)

	plan, err = vendorReload.Plan(rules)
	require.NoError(t, err)

	for _, m := range plan {
		require.False(t, m.Changed())
	}
}

func TestUpdateKeepsOldYaMakes(t *testing.T) {
	tmpArcadiaRoot := setupArcadia(t)

retry:
	m, err := gomod.CreateFakeModule(tmpArcadiaRoot)
	require.NoError(t, err)

	t.Logf("fake module is located in %s", m.Path)

	rules, err := yamake.ParsePolicy(bytes.NewBufferString(`
ALLOW .* -> vendor/rsc.io/quote/v3

ALLOW .* -> vendor/rsc.io/sampler
`))
	require.NoError(t, err)

	require.NoError(t, m.GoGet("rsc.io/quote/v3"))
	require.NoError(t, m.GoGet("rsc.io/sampler"))
	require.NoError(t, m.Save())

	vendor, err := NewVendor(tmpArcadiaRoot, m)
	require.NoError(t, err)

	plan, err := vendor.Plan(rules)
	if errors.Is(err, ErrGoModChanged) {
		require.NoError(t, m.Save())
		goto retry
	}

	require.NoError(t, err)

	for name, m := range plan {
		t.Logf("vendoring %s", name)
		require.NoError(t, vendor.VendorModule(m))
	}
	require.NoError(t, vendor.SaveModulesTXT())

	samplerGotestPath := filepath.Join(tmpArcadiaRoot, "vendor", "rsc.io", "sampler", "gotest", "ya.make")
	gotestYaMake, err := yamake.LoadYaMake(samplerGotestPath)
	require.NoError(t, err)

	gotestYaMake.Middle = []yamake.Macro{{Name: "SIZE", Args: []string{"MEDIUM"}}}
	require.NoError(t, gotestYaMake.Save(samplerGotestPath))

	for name, m := range plan {
		t.Logf("vendoring %s", name)
		require.NoError(t, vendor.VendorModule(m))
	}

	updatedGotestYaMake, err := yamake.LoadYaMake(samplerGotestPath)
	require.NoError(t, err)
	require.Equal(t, updatedGotestYaMake, gotestYaMake)
}

func TestYoIgnore(t *testing.T) {
	tmpArcadiaRoot := setupArcadia(t)

retry:
	m, err := gomod.CreateFakeModule(tmpArcadiaRoot)
	require.NoError(t, err)

	t.Logf("fake module is located in %s", m.Path)

	rules, err := yamake.ParsePolicy(bytes.NewBufferString(`ALLOW .* -> vendor/rsc.io/quote/v3`))
	require.NoError(t, err)

	require.NoError(t, m.GoGet("rsc.io/quote/v3"))
	require.NoError(t, m.Save())

	vendor, err := NewVendor(tmpArcadiaRoot, m)
	require.NoError(t, err)

	plan, err := vendor.Plan(rules)
	if errors.Is(err, ErrGoModChanged) {
		require.NoError(t, m.Save())
		goto retry
	}

	require.NoError(t, err)

	for name, m := range plan {
		t.Logf("vendoring %s", name)
		require.NoError(t, vendor.VendorModule(m))
	}
	require.NoError(t, vendor.SaveModulesTXT())

	yoignorePath := filepath.Join(tmpArcadiaRoot, "vendor", "golang.org", "x", "text", ".yoignore")
	yoignoreFile := `
*.json
message/pipeline/
`
	require.NoError(t, ioutil.WriteFile(yoignorePath, []byte(yoignoreFile), 0666))

	for name, m := range plan {
		t.Logf("vendoring %s", name)
		require.NoError(t, vendor.VendorModule(m))
	}
	require.NoError(t, vendor.SaveModulesTXT())

	pipelinePath := filepath.Join(tmpArcadiaRoot, "vendor", "golang.org", "x", "text", "message", "pipeline", "testdata", "ssa", "extracted.gotext.json")
	_, err = os.Stat(pipelinePath)
	require.Error(t, err)
}

func TestDeeplyNestedModules(t *testing.T) {
	tmpArcadiaRoot := setupArcadia(t)

	m, err := gomod.CreateFakeModule(tmpArcadiaRoot)
	require.NoError(t, err)
	t.Logf("fake module is located in %s", m.Path)

	rules, err := yamake.ParsePolicy(bytes.NewBufferString(`
ALLOW .* -> vendor/github.com/go-gl/glfw

ALLOW .* -> vendor/github.com/go-gl/glfw/v3.3/glfw
	`))
	require.NoError(t, err)

	require.NoError(t, m.GoGet("github.com/go-gl/glfw/v3.3/glfw"))
	require.NoError(t, m.GoGet("github.com/go-gl/glfw"))
	require.NoError(t, m.Save())

	vendor, err := NewVendor(tmpArcadiaRoot, m)
	require.NoError(t, err)

	plan, err := vendor.Plan(rules)
	require.NoError(t, err)

	require.NoError(t, vendor.VendorModule(plan["github.com/go-gl/glfw"]))

	_, err = os.Stat(filepath.Join(tmpArcadiaRoot, "vendor", "github.com/go-gl/glfw/v3.3/glfw"))
	require.Truef(t, os.IsNotExist(err), "%v", err)

	require.NoError(t, vendor.VendorModule(plan["github.com/go-gl/glfw/v3.3/glfw"]))
	require.NoError(t, vendor.SaveModulesTXT())

	_, err = os.Stat(filepath.Join(tmpArcadiaRoot, "vendor", "github.com/go-gl/glfw/v3.3/glfw"))
	require.NoError(t, err)
}
