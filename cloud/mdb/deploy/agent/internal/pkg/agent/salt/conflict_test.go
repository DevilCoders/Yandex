package salt_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/salt"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
)

func TestConflict(t *testing.T) {
	hs := commander.Command{Name: "state.highstate"}
	concurrentStateSLS := commander.Command{Name: "state.sls", Args: []string{"operations.metatada", "concurrent=True"}}
	stateSLS := commander.Command{Name: "state.sls", Args: []string{"operations.errata-apply"}}
	testNop := commander.Command{Name: "test.nop"}
	for _, cmd := range []commander.Command{hs, stateSLS} {
		t.Run(fmt.Sprintf("no conflict for %s(%s) when there are no running commands", cmd.Name, cmd.Args), func(t *testing.T) {
			require.False(t, salt.Conflict(cmd, nil))
		})
	}
	for _, cmd := range []commander.Command{hs, stateSLS} {
		t.Run(fmt.Sprintf("no conflict for %s(%s) and running non conflicting", cmd.Name, cmd.Args), func(t *testing.T) {
			require.False(t, salt.Conflict(cmd, []commander.Command{testNop}))
		})
	}
	t.Run("no conflict between running concurrent state.sls and new one", func(t *testing.T) {
		require.False(t, salt.Conflict(concurrentStateSLS, []commander.Command{concurrentStateSLS}))
	})
	for _, cmd := range []commander.Command{hs, stateSLS, concurrentStateSLS} {
		t.Run(fmt.Sprintf("%s(%s) conflicts with running highstate", cmd.Name, cmd.Args), func(t *testing.T) {
			require.True(t, salt.Conflict(cmd, []commander.Command{hs}))
		})
	}
	for _, cmd := range []commander.Command{hs, stateSLS, concurrentStateSLS} {
		t.Run(fmt.Sprintf("%s(%s) conflicts with running state.sls (without concurrent)", cmd.Name, cmd.Args), func(t *testing.T) {
			require.True(t, salt.Conflict(cmd, []commander.Command{stateSLS}))
		})
	}

	t.Run("conflict between versions", func(t *testing.T) {
		require.True(t, salt.Conflict(
			commander.Command{Name: "test.nop", Source: commander.DataSource{Version: "r3"}},
			[]commander.Command{{Name: "test.nop", Source: commander.DataSource{Version: "r1"}}},
		))
	})
}
