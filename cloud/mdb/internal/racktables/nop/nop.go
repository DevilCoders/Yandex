package nop

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/racktables"
)

type Client struct{}

var _ racktables.Client = &Client{}

func (c *Client) GetMacro(ctx context.Context, name string) (racktables.Macro, error) {
	return racktables.Macro{}, nil
}

func (c *Client) GetMacrosWithOwners(ctx context.Context) (racktables.MacrosWithOwners, error) {
	return racktables.MacrosWithOwners{}, nil
}
