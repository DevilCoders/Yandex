package kikimr

import (
	"testing"

	"github.com/BurntSushi/toml"
	"github.com/stretchr/testify/assert"
)

var conf = Config{
	"db1": DatabaseConfig{
		DBHost: "host1",
		Root:   "Root/",
	},
	"db2": DatabaseConfig{
		DBHost: "host2",
		Root:   "Root/Home",
	},
}

func TestToml(t *testing.T) {
	assert := assert.New(t)

	c := struct {
		Kikimr Config
	}{}

	_, err := toml.Decode(`
[kikimr]
[kikimr.db1]
DBHost = "host1"
Root = "Root/"
[kikimr.db2]
DBHost = "host2"
Root = "Root/Home"
`, &c)
	assert.NoError(err)
	assert.Equal(conf, c.Kikimr)
}

func TestTables(t *testing.T) {
	assert := assert.New(t)

	p, err := NewDatabaseProvider(conf)
	assert.NoError(err)
	assert.NotNil(p)
	assert.Equal(conf["db1"], p.GetDatabase("db1").GetConfig())
	assert.Equal(conf["db2"], p.GetDatabase("db2").GetConfig())

	assert.NoError(p.CreateFakeTables("db1/t1", "db1/t2", "db2/t1"))
	assert.Error(p.CreateFakeTables("db3"))
	assert.Error(p.CreateFakeTables("db3/t1"))

	assert.NoError(p.Verify("db1/t1", "db1/t2", "db2/t1"))
	assert.Error(p.Verify("db3"))
	assert.Error(p.Verify("db3/t1"))
	assert.Error(p.Verify("db1/t3"))

	assert.Equal("`Root/t1`", p.GetDatabase("db1").GetTable("t1").GetName())
	assert.Equal("`Root/Home/t1`", p.GetDatabase("db2").GetTable("t1").GetName())
}
