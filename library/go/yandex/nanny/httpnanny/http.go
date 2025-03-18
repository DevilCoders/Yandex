package httpnanny

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"regexp"
	"strconv"
	"strings"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/yandex/nanny"
)

const (
	apiEndpoint    = "nanny.yandex-team.ru"
	devAPIEndpoint = "dev-nanny.yandex-team.ru"
)

var _ nanny.Client = (*Client)(nil)

func New(opts ...Option) (*Client, error) {
	c := &Client{
		r:           resty.New(),
		apiEndpoint: apiEndpoint,
	}

	for _, opt := range opts {
		opt(c)
	}

	return c, nil
}

type Client struct {
	r *resty.Client

	token       string
	apiEndpoint string
}

func (c *Client) do(ctx context.Context, method, path string, params map[string]string, req, rsp interface{}) error {
	httpReq := c.r.R().
		SetContext(ctx).
		SetHeader("Authorization", "OAuth "+c.token)

	if params != nil {
		httpReq.SetQueryParams(params)
	}

	if req != nil {
		httpReq.SetBody(req)
	}

	if rsp != nil {
		httpReq.SetResult(&rsp)
	}

	if !strings.HasSuffix(path, "/") {
		return fmt.Errorf("nanny: request url must end with '/': got %q", path)
	}

	httpRsp, err := httpReq.Execute(method, "https://"+c.apiEndpoint+path)
	if err != nil {
		return &nanny.MethodError{Method: method, Path: path, Inner: err}
	}

	if httpRsp.IsError() {
		var validationError nanny.ValidationError
		_ = json.Unmarshal(httpRsp.Body(), &validationError)

		if validationError.Path != nil {
			return &nanny.MethodError{Method: method, Path: path, Inner: &validationError}
		}

		var apiErr nanny.APIError
		_ = json.Unmarshal(httpRsp.Body(), &apiErr) // API does not set proper content-type on errors.
		apiErr.StatusCode = httpRsp.StatusCode()

		return &nanny.MethodError{Method: method, Path: path, Inner: &apiErr}
	}

	return nil
}

func listParams(o nanny.ListOptions) map[string]string {
	p := map[string]string{}
	if o.ExcludeRuntimeAttrs {
		p["exclude_runtime_attrs"] = "1"
	}
	if o.Category != "" {
		p["category"] = o.Category
	}
	if o.Skip != 0 {
		p["skip"] = strconv.Itoa(o.Skip)
	}
	if o.Limit != 0 {
		p["limit"] = strconv.Itoa(o.Limit)
	}
	return p
}

func (c *Client) ListServices(ctx context.Context, opts nanny.ListOptions) ([]nanny.Service, error) {
	var rsp struct {
		Result []nanny.Service `json:"result"`
	}

	if err := c.do(ctx, http.MethodGet, "/v2/services/", listParams(opts), nil, &rsp); err != nil {
		return nil, err
	}

	return rsp.Result, nil
}

var serviceNameRE = regexp.MustCompile(`^[a-z0-9A-Z_\-]+$`)

func checkServiceID(serviceID string) error {
	if !serviceNameRE.MatchString(serviceID) {
		return fmt.Errorf("invalid service id: %q", serviceID)
	}
	return nil
}

func (c *Client) GetService(ctx context.Context, serviceID string) (*nanny.Service, error) {
	if err := checkServiceID(serviceID); err != nil {
		return nil, err
	}

	var result nanny.Service
	if err := c.do(ctx, http.MethodGet, "/v2/services/"+serviceID+"/", nil, nil, &result); err != nil {
		return nil, err
	}

	return &result, nil
}

func (c *Client) CreateService(ctx context.Context, service *nanny.ServiceSpec, comment string) (*nanny.Service, error) {
	request := struct {
		ID           string              `json:"id"`
		Comment      string              `json:"comment"`
		InfoAttrs    *nanny.InfoAttrs    `json:"info_attrs"`
		RuntimeAttrs *nanny.RuntimeAttrs `json:"runtime_attrs"`
		AuthAttrs    *nanny.AuthAttrs    `json:"auth_attrs,omitempty"`
	}{
		service.ID, comment, service.InfoAttrs, service.RuntimeAttrs, service.AuthAttrs,
	}

	var result nanny.Service

	if err := c.do(ctx, http.MethodPost, "/v2/services/", nil, request, &result); err != nil {
		return nil, err
	}

	return &result, nil
}

func (c *Client) DeleteService(ctx context.Context, serviceID string) error {
	if err := checkServiceID(serviceID); err != nil {
		return err
	}

	return c.do(ctx, http.MethodDelete, "/v2/services/"+serviceID+"/", nil, nil, nil)
}

func (c *Client) GetInfoAttrs(ctx context.Context, serviceID string) (*nanny.InfoSnapshot, error) {
	if err := checkServiceID(serviceID); err != nil {
		return nil, err
	}

	var result nanny.InfoSnapshot
	if err := c.do(ctx, http.MethodGet, "/v2/services/"+serviceID+"/info_attrs/", nil, nil, &result); err != nil {
		return nil, err
	}
	return &result, nil
}

func (c *Client) UpdateInfoAttrs(ctx context.Context, serviceID string, attr *nanny.InfoAttrs, update *nanny.UpdateInfo) (*nanny.InfoSnapshot, error) {
	if err := checkServiceID(serviceID); err != nil {
		return nil, err
	}

	var result nanny.InfoSnapshot
	if err := c.do(ctx, http.MethodPut, "/v2/services/"+serviceID+"/info_attrs/", nil, update.Commit(attr), nil); err != nil {
		return nil, err
	}
	return &result, nil
}

func (c *Client) GetRuntimeAttrs(ctx context.Context, serviceID string) (*nanny.RuntimeSnapshot, error) {
	if err := checkServiceID(serviceID); err != nil {
		return nil, err
	}

	var result nanny.RuntimeSnapshot
	if err := c.do(ctx, http.MethodGet, "/v2/services/"+serviceID+"/runtime_attrs/", nil, nil, &result); err != nil {
		return nil, err
	}
	return &result, nil
}

func (c *Client) UpdateRuntimeAttrs(ctx context.Context, serviceID string, attr *nanny.RuntimeAttrs, update *nanny.UpdateInfo) (*nanny.RuntimeSnapshot, error) {
	if err := checkServiceID(serviceID); err != nil {
		return nil, err
	}

	var result nanny.RuntimeSnapshot
	if err := c.do(ctx, http.MethodPut, "/v2/services/"+serviceID+"/runtime_attrs/", nil, update.Commit(attr), &result); err != nil {
		return nil, err
	}
	return &result, nil
}

func (c *Client) GetAuthAttrs(ctx context.Context, serviceID string) (*nanny.AuthSnapshot, error) {
	if err := checkServiceID(serviceID); err != nil {
		return nil, err
	}

	var result nanny.AuthSnapshot
	if err := c.do(ctx, http.MethodGet, "/v2/services/"+serviceID+"/auth_attrs/", nil, nil, &result); err != nil {
		return nil, err
	}
	return &result, nil
}

func (c *Client) UpdateAuthAttrs(ctx context.Context, serviceID string, attr *nanny.AuthAttrs, update *nanny.UpdateInfo) (*nanny.AuthSnapshot, error) {
	if err := checkServiceID(serviceID); err != nil {
		return nil, err
	}

	var result nanny.AuthSnapshot
	if err := c.do(ctx, http.MethodPut, "/v2/services/"+serviceID+"/auth_attrs/", nil, update.Commit(attr), &result); err != nil {
		return nil, err
	}
	return &result, nil
}
