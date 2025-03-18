package main

import (
	"context"

	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb/bbtypes"
)

var _ blackbox.Client = (*BlackBoxClient)(nil)
var _ Client = (*BlackBoxClient)(nil)

type Client interface {
	blackbox.Client
}

type BlackBoxClient struct {
	*httpbb.Client
}

var _ bbtypes.Response = (*GetMaxUIDResponse)(nil)

type GetMaxUIDResponse struct {
	bbtypes.BaseResponse
	OK bool
}

func NewBlackBoxClient(env httpbb.Environment, opts ...httpbb.Option) (*BlackBoxClient, error) {
	bbClient, err := httpbb.NewClient(env, opts...)
	if err != nil {
		return nil, err
	}

	return &BlackBoxClient{
		Client: bbClient,
	}, nil
}

func (c *BlackBoxClient) GetMaxUID(ctx context.Context) (*GetMaxUIDResponse, error) {
	httpReq := c.InternalHTTP().R(ctx).SetQueryParams(map[string]string{
		"method": "get_max_uid",
		"format": "json",
	})

	var rsp GetMaxUIDResponse
	if err := c.InternalHTTP().Get(httpReq, &rsp); err != nil {
		return nil, err
	}

	return &rsp, nil
}
