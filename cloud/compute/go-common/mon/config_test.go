package mon

import (
	"encoding/json"
	"testing"
	"time"

	"github.com/BurntSushi/toml"
	"github.com/stretchr/testify/assert"
)

func TestConfig(t *testing.T) {
	assert := assert.New(t)
	var cfg struct {
		Mon Config `toml:"monitoring" json:"monitoring"`
	}

	tomlConfig := `
[monitoring]
timeout="10m"
`
	jsonConfig := `
{
    "monitoring": {"timeout": "10m"}
}
    `
	assert.Empty(cfg.Mon.RepositoryOptions())

	_, err := toml.Decode(tomlConfig, &cfg)
	assert.NoError(err)
	assert.Equal(10*time.Minute, time.Duration(cfg.Mon.Timeout))

	cfg.Mon.Timeout = Duration(time.Second * 0)
	assert.NoError(json.Unmarshal([]byte(jsonConfig), &cfg))
	assert.Equal(10*time.Minute, time.Duration(cfg.Mon.Timeout))
	assert.Len(cfg.Mon.RepositoryOptions(), 1)

	badTomlConfig := `
[monitoring]
timeout="10"
`
	_, err = toml.Decode(badTomlConfig, &cfg)
	assert.Error(err)
}
