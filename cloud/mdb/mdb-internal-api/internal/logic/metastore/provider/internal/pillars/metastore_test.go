package pillars

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestSetVersionOnCreate(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		pillar := NewCluster()
		err := pillar.SetVersion("3.0.0")
		require.NoError(t, err)
		require.Equal(t, "3.0.0", pillar.Data.Version)
	})

	t.Run("UseDefault", func(t *testing.T) {
		pillar := NewCluster()
		err := pillar.SetVersion("")
		require.NoError(t, err)
		require.Equal(t, "3.0.0", pillar.Data.Version)
	})

	t.Run("UnknownVersion", func(t *testing.T) {
		pillar := NewCluster()
		err := pillar.SetVersion("5.0.0")
		require.EqualError(t, err, "unknown Metastore version")
	})
}
