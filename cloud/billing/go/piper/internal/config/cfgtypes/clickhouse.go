package cfgtypes

type Clickhouse struct {
	ClickhouseCommonConfig `yaml:",inline"`
	Shards                 []ClickhouseShard `yaml:"shards" config:"shards"`
}

type ClickhouseCommonConfig struct {
	Database   string          `yaml:"database" config:"database"`
	DisableTLS OverridableBool `yaml:"disable_tls,omitempty" config:"disable_tls"`
	Port       int             `yaml:"port,omitempty" config:"port"`

	Auth ClickhouseAuth `yaml:"auth" config:"auth"`

	MaxConnections     int     `yaml:"max_connections,omitempty" config:"max_connections"`
	MaxIdleConnections int     `yaml:"max_idle_connections,omitempty" config:"max_idle_connections"`
	ConnMaxLifetime    Seconds `yaml:"conn_max_lifetime,omitempty" config:"conn_max_lifetime"`
}

type ClickhouseAuth struct {
	User     string `yaml:"user,omitempty" config:"user"`
	Password string `yaml:"password,omitempty" config:"password"`
}

type ClickhouseShard struct {
	Hosts []string `yaml:"hosts,omitempty" config:"hosts"`
}
