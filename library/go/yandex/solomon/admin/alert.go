package admin

import (
	"context"

	"a.yandex-team.ru/library/go/yandex/solomon"
)

func (c *Client) ListAlerts(ctx context.Context, opts ...solomon.ListOption) ([]solomon.AlertDescription, error) {
	var alerts []solomon.AlertDescription
	err := c.ListObjects(ctx, solomon.TypeAlert, &alerts, opts...)
	return alerts, err
}

func (c *Client) FindAlert(ctx context.Context, id string) (solomon.Alert, error) {
	var alert solomon.Alert
	err := c.FindObject(ctx, solomon.TypeAlert, id, &alert)
	return alert, err
}

func (c *Client) DeleteAlert(ctx context.Context, id string) error {
	return c.DeleteObject(ctx, solomon.TypeAlert, id)
}

func (c *Client) CreateAlert(ctx context.Context, alert solomon.Alert) (solomon.Alert, error) {
	err := c.CreateObject(ctx, solomon.TypeAlert, &alert)
	return alert, err
}

func (c *Client) UpdateAlert(ctx context.Context, alert solomon.Alert) (solomon.Alert, error) {
	err := c.UpdateObject(ctx, solomon.TypeAlert, alert.ID, &alert)
	return alert, err
}
