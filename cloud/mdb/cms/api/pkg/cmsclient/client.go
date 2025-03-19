package cmsclient

import (
	"context"
	"net/url"

	client2 "a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/client"
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/client/stability"
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	swagclient "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/client"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil/openapi"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/library/go/core/log"
)

type Client interface {
	ready.Checker

	GetTasks(ctx context.Context) (*models.TasksResultsArray, error)
	DeleteTask(ctx context.Context, taskID string) error
}

type client struct {
	swag   *client2.MdbCmsapi
	l      log.Logger
	ticket string
}

func (c *client) IsReady(ctx context.Context) error {
	_, err := c.swag.Stability.Ping(stability.NewPingParamsWithContext(ctx))
	if err == nil {
		return nil
	}

	return apiError(err)
}

func New(uri, ticket string, tlscfg httputil.TLSConfig, logcfg httputil.LoggingConfig, l log.Logger) (Client, error) {
	u, err := url.Parse(uri)
	if err != nil {
		return nil, err
	}

	rt, err := httputil.DEPRECATEDNewTransport(tlscfg, logcfg, l)
	if err != nil {
		return nil, err
	}

	crt := openapi.NewRuntime(
		u.Host,
		swagclient.DefaultBasePath,
		[]string{u.Scheme},
		rt,
		l,
	)

	return &client{
		l:      l,
		swag:   client2.New(crt, nil),
		ticket: ticket,
	}, nil
}
