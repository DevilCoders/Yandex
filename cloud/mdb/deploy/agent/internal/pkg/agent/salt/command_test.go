package salt

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
)

func TestFormCommand(t *testing.T) {

	t.Run("for a valid command", func(t *testing.T) {
		cfg := DefaultConfig()
		cfg.Binary = "test-salt-call"
		cfg.Args = []string{"--foo"}

		ret, err := FormCommand(
			cfg,
			commander.Command{
				ID:   "id1",
				Name: "state.highstate",
				Args: []string{"pillar={}"},
			},
		)
		require.NoError(t, err)
		require.Equal(t,
			Job{
				ID:   "id1",
				Name: "test-salt-call",
				Args: []string{"--foo", "state.highstate", "pillar={}"},
			},
			ret)
	})

	for _, cmd := range []commander.Command{
		{Name: "--failhard"},
		{Name: "state.highstate", Args: []string{"--failhard"}},
	} {
		t.Run(fmt.Sprintf("Invalid %+v", cmd), func(t *testing.T) {
			_, err := FormCommand(DefaultConfig(), cmd)
			require.Error(t, err)
		})
	}
}
