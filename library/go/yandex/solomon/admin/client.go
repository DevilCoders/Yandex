package admin

import (
	"context"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/yandex/solomon"
)

var _ solomon.AdminClient = (*Client)(nil)

type Client struct {
	solomonURL string
	oauthToken string
	project    string

	c *resty.Client
}

func New(project string, oauthToken string, opts ...Option) *Client {
	client := &Client{
		solomonURL: ProductionURL,
		project:    project,
		oauthToken: oauthToken,
		c:          resty.New(),
	}

	for _, opt := range opts {
		opt(client)
	}

	client.c.SetHeader("Authorization", "OAuth "+oauthToken)
	client.c.SetBaseURL(client.solomonURL + "/projects/" + client.project)

	return client
}

func (c *Client) r(ctx context.Context) *resty.Request {
	return c.c.R().
		SetContext(ctx).
		ExpectContentType("application/json")
}
