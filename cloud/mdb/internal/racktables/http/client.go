package http

import (
	"context"
	"fmt"
	"net/http"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/racktables"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	config Config
	client *httputil.Client
}

type Config struct {
	Endpoint string                `json:"endpoint" yaml:"endpoint"`
	Client   httputil.ClientConfig `json:"client" yaml:"client"`
	Token    secret.String         `json:"token" yaml:"token"`
}

func DefaultConfig() Config {
	return Config{
		Client: httputil.ClientConfig{
			Name:      "MDB Racktables Client",
			Transport: httputil.DefaultTransportConfig(),
		},
	}
}

func NewClient(config Config, logger log.Logger) (*Client, error) {
	client, err := httputil.NewClient(config.Client, logger)
	if err != nil {
		return nil, err
	}
	return &Client{
		config: config,
		client: client,
	}, nil
}

var _ racktables.Client = &Client{}

func (c *Client) get(ctx context.Context, url string, opName string, tags ...opentracing.Tag) (*http.Response, error) {
	req, err := http.NewRequestWithContext(ctx, "GET", url, nil)
	if err != nil {
		return nil, xerrors.Errorf("create new request to racktables: %w", err)
	}

	req.Header.Set("Authorization", fmt.Sprintf("OAuth %s", c.config.Token.Unmask()))

	return c.client.Do(req, opName, tags...)
}
