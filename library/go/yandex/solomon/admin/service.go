package admin

import (
	"context"

	"a.yandex-team.ru/library/go/yandex/solomon"
)

func (c *Client) FindService(ctx context.Context, id string) (solomon.Service, error) {
	var service solomon.Service
	err := c.FindObject(ctx, solomon.TypeService, id, &service)
	return service, err
}

func (c *Client) CreateService(ctx context.Context, service solomon.Service) (solomon.Service, error) {
	err := c.CreateObject(ctx, solomon.TypeService, &service)
	return service, err
}

func (c *Client) ListServices(ctx context.Context, opts ...solomon.ListOption) ([]solomon.Service, error) {
	var services []solomon.Service
	err := c.ListObjects(ctx, solomon.TypeService, &services, opts...)
	return services, err
}

func (c *Client) UpdateService(ctx context.Context, service solomon.Service) (solomon.Service, error) {
	err := c.UpdateObject(ctx, solomon.TypeService, service.ID, &service)
	return service, err
}

func (c *Client) DeleteService(ctx context.Context, id string) error {
	return c.DeleteObject(ctx, solomon.TypeService, id)
}
