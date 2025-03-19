package cfgtypes

type LbInstallationConfig struct {
	Host       string          `yaml:"host" config:"host"`
	Port       int             `yaml:"port" config:"port"`
	Database   string          `yaml:"database" config:"database"`
	DisableTLS OverridableBool `yaml:"disable_tls,omitempty" config:"disable_tls"`

	Auth AuthConfig `yaml:"auth" config:"auth"`

	ResharderConsumer string `yaml:"resharder_consumer" config:"resharder_consumer"`
}
