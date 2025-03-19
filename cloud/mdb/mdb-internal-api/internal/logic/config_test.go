package logic

import (
	"testing"

	"github.com/stretchr/testify/require"
	"gopkg.in/yaml.v2"
)

func TestConfig(t *testing.T) {
	t.Run("Monitoring", func(t *testing.T) {
		input := []byte(
			"charts:\n" +
				"  postgresql:\n" +
				"  - name: foo\n" +
				"    description: bar\n" +
				"    link: foobar\n",
		)
		var cfg MonitoringConfig
		require.NoError(t, yaml.Unmarshal(input, &cfg))
	})
}
