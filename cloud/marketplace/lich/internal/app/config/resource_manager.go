package config

type ResourceManager struct {
	Endpoint string `config:"endpoint" yaml:"endpoint"`
	CAPath   string `config:"ca-path" yaml:"ca-path"`
}
