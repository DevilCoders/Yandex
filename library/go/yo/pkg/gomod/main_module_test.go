package gomod_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yo/pkg/gomod"
	"a.yandex-team.ru/library/go/yo/pkg/testutil"
)

func TestModuleDownload(t *testing.T) {
	testutil.NeedProxyAccess(t)

	m, err := gomod.CreateFakeModule(testutil.ArcadiaRoot)
	require.NoError(t, err)

	require.NoError(t, m.GoGet("rsc.io/quote/v3"))
	require.NoError(t, m.GoGet("rsc.io/quote"))

	deps, err := m.DownloadModules()
	require.NoError(t, err)

	modules := map[string]struct{}{}
	for _, dep := range deps {
		modules[dep.Path] = struct{}{}
	}

	require.Contains(t, modules, "rsc.io/quote")
	require.Contains(t, modules, "rsc.io/quote/v3")
}

func TestModuleUpgrade(t *testing.T) {
	testutil.NeedProxyAccess(t)

	m, err := gomod.CreateFakeModule(testutil.ArcadiaRoot)
	require.NoError(t, err)

	require.NoError(t, m.GoGet("rsc.io/quote/v3@v3.0.0"))

	deps, err := m.ListModuleUpdates()
	require.NoError(t, err)

	for _, dep := range deps {
		if dep.Path == "rsc.io/quote/v3" {
			require.NotNil(t, dep.Update, dep)
		}
	}
}
