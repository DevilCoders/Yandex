package config

type YDB struct {
	Endpoint string `config:"endpoint" yaml:"endpoint"`

	DBRoot   string `config:"db-root"  yaml:"db-root"`
	Database string `config:"database" yaml:"database"`

	CAPath string `config:"ca-path"  yaml:"ca-path"`
}
