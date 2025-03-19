package config

type HTTPService struct {
	ListenEndpoint string `config:"listen-endpoint" yaml:"listen-endpoint"`
}
