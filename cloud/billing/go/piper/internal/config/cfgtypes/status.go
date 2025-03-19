package cfgtypes

type StatusServerConfig struct {
	Enabled OverridableBool `yaml:"enabled,omitempty" config:"enabled"`
	Port    int             `yaml:"port,omitempty" config:"port"`
}
