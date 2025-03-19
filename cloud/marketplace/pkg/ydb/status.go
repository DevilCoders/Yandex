package ydb

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/pkg/monitoring/status"
)

type StatusChecker interface {
	Check(context.Context) status.Status
	Ping(context.Context) error
}

const (
	statusProviderName = "yc-marketplace-ydb"
)

func (c *Connector) Ping(ctx context.Context) error {
	return c.db.PingContext(ctx)
}

func (c *Connector) Check(ctx context.Context) status.Status {
	if err := c.Ping(ctx); err != nil {
		return c.newErrorStatus(err)
	}

	return c.newNormalStatus()
}

func (c *Connector) newNormalStatus() status.Status {
	return status.Status{
		Name:        statusProviderName,
		Description: "PASSED: None",

		Code: status.StatusCodeOK,
	}
}

func (c *Connector) newErrorStatus(err error) status.Status {
	if err == nil {
		c.newNormalStatus()
	}

	return status.Status{
		Name:        statusProviderName,
		Description: fmt.Sprintf("FAILED: %s", err),

		Code: status.StatusCodeError,
	}
}
