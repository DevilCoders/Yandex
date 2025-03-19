package cfgtypes

type ResharderConfig struct {
	Disabled              OverridableBool `yaml:"disabled" config:"disabled"`
	EnableTeamIntegration OverridableBool `yaml:"enable_team_integration" config:"enable_team_integration"`
	EnableDeduplication   OverridableBool `yaml:"enable_deduplication" config:"enable_deduplication"`
	EnableE2EReporting    OverridableBool `yaml:"enable_e2e_reporting" config:"enable_e2e_reporting"`

	Sink   ResharderSinkConfig               `yaml:"sink" config:"sink"`
	Source map[string]*ResharderSourceConfig `yaml:"source" config:"source"`

	MetricsGrace Seconds `yaml:"metrics_grace" config:"metrics_grace"`
	WriteLimit   int     `yaml:"write_limit" config:"write_limit"`
}

type ResharderSinkConfig struct {
	Logbroker       LogbrokerSinkConfig `yaml:"logbroker" config:"logbroker"`
	LogbrokerErrors LogbrokerSinkConfig `yaml:"logbroker_errors" config:"logbroker_errors"`
	YDBErrors       YDBSinkConfig       `yaml:"ydb_errors" config:"ydb_errors"`
}

type LogbrokerSinkConfig struct {
	Enabled      OverridableBool `yaml:"enabled" config:"enabled"`
	Installation string          `yaml:"installation" config:"installation"`
	Topic        string          `yaml:"topic" config:"topic"`
	Partitions   int             `yaml:"partitions" config:"partitions"`
	Route        string          `yaml:"route" config:"route"`

	MaxParallel int               `yaml:"max_parallel" config:"max_parallel"`
	SplitSize   DataSize          `yaml:"split_size" config:"split_size"`
	Compression CompressionConfig `yaml:"compression" config:"compression"`
}

type ResharderSourceConfig struct {
	Disabled  OverridableBool        `yaml:"disabled" config:"disabled"`
	Logbroker LogbrokerSourceConfig  `yaml:"logbroker" config:"logbroker"`
	Parallel  int                    `yaml:"parallel" config:"parallel"`
	Handler   string                 `yaml:"handler" config:"handler"`
	Params    ResharderHandlerConfig `yaml:"params" config:"params"`
}

type ResharderHandlerConfig struct {
	ChunkSize      DataSize `yaml:"chunk_size" config:"chunk_size"`
	MetricLifetime Seconds  `yaml:"metric_lifetime" config:"metric_lifetime"`
	MetricGrace    Seconds  `yaml:"metric_grace" config:"metric_grace"`
}
