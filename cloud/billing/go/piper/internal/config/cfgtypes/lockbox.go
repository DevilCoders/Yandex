package cfgtypes

type LockboxConfig struct {
	Endpont string          `yaml:"endpoint,omitempty" config:"endpoint"`
	Enabled OverridableBool `yaml:"enabled" config:"enabled"`
}
