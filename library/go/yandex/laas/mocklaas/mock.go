package mocklaas

import (
	"context"

	"a.yandex-team.ru/library/go/yandex/laas"
)

var _ laas.Client = new(Client)

type Client struct {
	MockDetectRegion func(context.Context, laas.Params) (*laas.RegionResponse, error)
}

func (c Client) DetectRegion(ctx context.Context, p laas.Params) (*laas.RegionResponse, error) {
	if c.MockDetectRegion == nil {
		return new(laas.RegionResponse), nil
	}
	return c.MockDetectRegion(ctx, p)
}
