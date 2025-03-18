package govendor

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yo/pkg/gomod"
	"a.yandex-team.ru/library/go/yo/pkg/testutil"
)

func TestModulePackages(t *testing.T) {
	testutil.NeedProxyAccess(t)

	m, err := gomod.CreateFakeModule(testutil.ArcadiaRoot)
	require.NoError(t, err)

	require.NoError(t, m.GoGet("golang.org/x/sys"))

	pkgs, err := GetImports(m, "golang.org/x/sys")
	require.NoError(t, err)

	require.Contains(t, pkgs, "golang.org/x/sys/windows")
	require.Contains(t, pkgs, "golang.org/x/sys/unix")

	require.NoError(t, m.GoGet("rsc.io/quote"))
	require.NoError(t, m.GoGet("rsc.io/quote/v3"))

	pkgs, err = GetImports(m, "rsc.io/quote")
	require.NoError(t, err)
	require.NotContains(t, pkgs, "rsc.io/quote/v3")
}
