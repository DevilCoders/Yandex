package restapi

import (
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
)

// Transport is transport config
type Transport struct {
	TLS     httputil.TLSConfig     `json:"tls" yaml:"tls"`
	Logging httputil.LoggingConfig `json:"logging" yaml:"logging"`
}

// Config is DMB client configuration
type Config struct {
	Host      string        `json:"host" yaml:"host"`
	Token     secret.String `json:"token" yaml:"token"`
	Transport Transport     `json:"transport" yaml:"transport"`
}

// DefaultConfig return default configuration
func DefaultConfig() Config {
	return Config{
		Host: "mdb.yandex-team.ru",
		Transport: Transport{
			Logging: httputil.DefaultLoggingConfig(),
		},
	}
}

// Client ...
type Client struct {
	httpClient *httputil.Client
	host       string
	token      string
	L          log.Logger
}

var _ dbm.Client = &Client{}

// New constructs DBM client
func New(cfg Config, L log.Logger) (*Client, error) {
	rt, err := httputil.DEPRECATEDNewTransport(cfg.Transport.TLS, cfg.Transport.Logging, L)
	if err != nil {
		return nil, err
	}

	return &Client{
			host:       cfg.Host,
			token:      cfg.Token.Unmask(),
			httpClient: httputil.DEPRECATEDNewClient(&http.Client{Transport: rt}, "DBM", L),
			L:          L},
		nil
}
