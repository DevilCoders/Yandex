package admin

import (
	"context"
	"fmt"
	"net/http"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/yandex/solomon"
)

func fillListOptions(req *resty.Request, opts ...solomon.ListOption) *solomon.NextPageToken {
	var next *solomon.NextPageToken

	for _, opt := range opts {
		switch o := opt.(type) {
		case solomon.PageSize:
			if o.Size == nil {
				req.SetQueryParam("pageSize", "all")
			} else {
				req.SetQueryParam("pageSize", fmt.Sprint(*o.Size))
			}

			if o.Page != 0 {
				req.SetQueryParam("page", fmt.Sprint(o.Page))
			}

		case solomon.PageToken:
			req.SetQueryParam("pageToken", o.Token)

		case solomon.AlertFilter:
			req.SetQueryParam("filterByName", o.FilterByName)

		case *solomon.NextPageToken:
			next = o

		default:
			panic(fmt.Sprintf("unexpected list option %T", opt))
		}
	}

	return next
}

func (c *Client) ListObjects(ctx context.Context, typ solomon.ObjectType, slice interface{}, opts ...solomon.ListOption) error {
	var apiErr solomon.APIError
	var result struct {
		Items  interface{} `json:"items"`
		Result interface{} `json:"result"`

		NextPageToken string `json:"nextPageToken"`
	}

	if typ == solomon.TypeAlert {
		result.Items = slice
	} else {
		result.Result = slice
	}

	req := c.r(ctx).
		SetError(&apiErr).
		SetResult(&result)
	nextToken := fillListOptions(req, opts...)

	rsp, err := req.
		SetPathParams(map[string]string{"type": string(typ)}).
		Get("/{type}")

	if err != nil {
		return err
	}

	if rsp.IsError() {
		return &apiErr
	}

	if nextToken != nil {
		nextToken.NextToken = result.NextPageToken
	}

	return nil
}

func (c *Client) FindObject(ctx context.Context, typ solomon.ObjectType, id string, o interface{}) error {
	var apiErr solomon.APIError

	rsp, err := c.r(ctx).
		SetError(&apiErr).
		SetResult(o).
		SetPathParams(map[string]string{"type": string(typ), "id": id}).
		Get("/{type}/{id}")

	if err != nil {
		return err
	}

	if rsp.StatusCode() == http.StatusNotFound {
		return solomon.ErrNotFound
	}

	if rsp.IsError() {
		return &apiErr
	}

	return nil
}

func (c *Client) CreateObject(ctx context.Context, typ solomon.ObjectType, o interface{}) error {
	var apiErr solomon.APIError

	rsp, err := c.r(ctx).
		SetError(&apiErr).
		SetBody(o).
		SetResult(o).
		SetPathParams(map[string]string{"type": string(typ)}).
		Post("/{type}")

	if err != nil {
		return err
	}

	if rsp.IsError() {
		return &apiErr
	}

	return nil
}

func (c *Client) UpdateObject(ctx context.Context, typ solomon.ObjectType, id string, o interface{}) error {
	var apiErr solomon.APIError

	rsp, err := c.r(ctx).
		SetError(&apiErr).
		SetBody(o).
		SetResult(o).
		SetPathParams(map[string]string{"type": string(typ), "id": id}).
		Put("/{type}/{id}")

	if err != nil {
		return err
	}

	if rsp.IsError() {
		return &apiErr
	}

	return nil
}

func (c *Client) DeleteObject(ctx context.Context, typ solomon.ObjectType, id string) error {
	var apiErr solomon.APIError

	rsp, err := c.r(ctx).
		SetError(&apiErr).
		SetPathParams(map[string]string{"type": string(typ), "id": id}).
		Delete("/{type}/{id}")

	if err != nil {
		return err
	}

	if rsp.IsError() {
		return &apiErr
	}

	return nil
}
