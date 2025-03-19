package cfgtypes

type DumperConfig struct {
	Disabled OverridableBool              `yaml:"disabled" config:"disabled"`
	Sink     DumpSinkConfig               `yaml:"sink" config:"sink"`
	Source   map[string]*DumpSourceConfig `yaml:"source" config:"source"`
}

type DumpSinkConfig struct {
	YDBErrors        YDBSinkConfig          `yaml:"ydb_errors" config:"ydb_errors"`
	ClickhouseErrors ClickhouseSinkConfig   `yaml:"ch_errors" config:"ch_errors"`
	YDBPresenter     YDBPresenterSinkConfig `yaml:"ydb_presenter" config:"ydb_presenter"`
}

type DumpSourceConfig struct {
	Disabled  OverridableBool       `yaml:"disabled" config:"disabled"`
	Logbroker LogbrokerSourceConfig `yaml:"logbroker" config:"logbroker"`
	Handler   string                `yaml:"handler" config:"handler"`
}
