package config

type GRPCService struct {
	ListenEndpoint string `config:"listen-endpoint" yaml:"listen-endpoint"`
	Port           int    `config:"port" yaml:"port"`
}
