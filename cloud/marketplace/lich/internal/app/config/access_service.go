package config

type AccessService struct {
	Endpoint string `config:"endpoint" yaml:"endpoint"`
	CAPath   string `config:"ca-path" yaml:"ca-path"`
}
