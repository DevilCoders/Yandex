package cfgtypes

type CompressionConfig struct {
	Disabled OverridableBool `yaml:"disabled" config:"disabled"`
	Level    int             `yaml:"level" config:"level"`
}

type ClickhouseSinkConfig struct {
	Enabled OverridableBool `yaml:"enabled" config:"enabled"`
}

type YDBSinkConfig struct {
	Enabled OverridableBool `yaml:"enabled" config:"enabled"`
}

type YDBPresenterSinkConfig struct {
	Enabled OverridableBool `yaml:"enabled" config:"enabled"`
}

type LogbrokerSourceConfig struct {
	Installation string `yaml:"installation" config:"installation"`
	Consumer     string `yaml:"consumer,omitempty" config:"consumer"`
	Topic        string `yaml:"topic" config:"topic"`

	MaxMessages int      `yaml:"max_messages" config:"max_messages"`
	MaxSize     DataSize `yaml:"max_size" config:"max_size"`
	Lag         Seconds  `yaml:"lag" config:"lag"`

	BatchLimit   int      `yaml:"batch_limit" config:"batch_limit"`
	BatchSize    DataSize `yaml:"batch_size" config:"batch_size"`
	BatchTimeout Seconds  `yaml:"batch_timeout" config:"batch_timeout"`
}
