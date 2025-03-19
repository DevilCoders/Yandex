package cfgtypes

type YDBConfig struct {
	YDBParams     `yaml:",inline"`
	Installations YDBInstallations `yaml:"installations" config:"installations"`
}

type YDBInstallations struct {
	Uniq       YDBParams `yaml:"uniq" config:"uniq"`
	Cumulative YDBParams `yaml:"cumulative" config:"cumulative"`
	Presenter  YDBParams `yaml:"presenter" config:"presenter"`
}

type YDBParams struct {
	Address    string          `yaml:"address" config:"address"`
	Database   string          `yaml:"database" config:"database"`
	Root       string          `yaml:"root" config:"root"`
	DisableTLS OverridableBool `yaml:"disable_tls,omitempty" config:"disable_tls"`

	Auth AuthConfig `yaml:"auth" config:"auth"`

	ConnectTimeout       Seconds `yaml:"connect_timeout,omitempty" config:"connect_timeout"`
	RequestTimeout       Seconds `yaml:"request_timeout,omitempty" config:"request_timeout"`
	DiscoveryInterval    Seconds `yaml:"discovery_interval,omitempty" config:"discovery_interval"`
	MaxConnections       int     `yaml:"max_connections,omitempty" config:"max_connections"`
	MaxIdleConnections   int     `yaml:"max_idle_connections,omitempty" config:"max_idle_connections"`
	MaxDirectConnections int     `yaml:"max_direct_connections,omitempty" config:"max_direct_connections"`
	ConnMaxLifetime      Seconds `yaml:"conn_max_lifetime,omitempty" config:"conn_max_lifetime"`
}
