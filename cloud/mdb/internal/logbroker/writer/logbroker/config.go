package logbroker

import (
	"github.com/c2h5oh/datasize"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
)

type OAuthConfig struct {
	Token secret.String `json:"token" yaml:"token"`
}

type TVMConfig struct {
	Token          secret.String `json:"token" yaml:"token"`
	ClientAlias    string        `json:"client_alias" yaml:"client_alias"`
	LogBrokerAlias string        `json:"logbroker_alias" yaml:"logbroker_alias"`
	Port           int           `json:"port" yaml:"port"`
}

type IAMConfig struct {
	TokenService struct {
		Endpoint  string `json:"endpoint" yaml:"endpoint"`
		UserAgent string `json:"useragent" yaml:"useragent"`
	} `json:"token_service" yaml:"token_service"`
	ServiceAccount struct {
		ID         string        `json:"id" yaml:"id"`
		KeyID      secret.String `json:"key_id" yaml:"key_id"`
		PrivateKey secret.String `json:"private_key" yaml:"private_key"`
	} `json:"service_account" yaml:"service_account"`
}

type TLSConfig struct {
	RootCAFile string `json:"ca_file" yaml:"ca_file"`
}

// Config ...
type Config struct {
	Endpoint  string            `json:"endpoint" yaml:"endpoint"`
	Database  string            `json:"database" yaml:"database"`
	Topic     string            `json:"topic" yaml:"topic"`
	SourceID  string            `json:"source_id" yaml:"source_id"`
	MaxMemory datasize.ByteSize `json:"max_memory" yaml:"max_memory"`
	TLS       TLSConfig         `json:"tls" yaml:"tls"`
	OAuth     OAuthConfig       `json:"oauth" yaml:"oauth"`
	TVM       TVMConfig         `json:"tvm" yaml:"tvm"`
	IAM       IAMConfig         `json:"iam" yaml:"iam"`
}

// DefaultConfig fill config with defaults
func DefaultConfig() Config {
	return Config{
		MaxMemory: datasize.GB,
		TVM: TVMConfig{
			LogBrokerAlias: "logbroker",
		},
	}
}
