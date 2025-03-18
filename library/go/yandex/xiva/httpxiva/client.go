package httpxiva

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/xiva"
)

var _ xiva.Client = new(Client) // check correct implementation faster

type Client struct {
	host              string
	subscriptionToken string
	sendToken         string
	serviceName       string
	httpc             *resty.Client
}

const (
	defaultHTTPHost = "https://push.yandex.ru"
)

func NewClient(opts ...Option) (*Client, error) {
	return NewClientWithResty(resty.New(), opts...)
}

func NewClientWithResty(restyClient *resty.Client, opts ...Option) (*Client, error) {
	c := &Client{
		httpc: restyClient,
	}

	for _, opt := range opts {
		if err := opt(c); err != nil {
			return nil, err
		}
	}

	if c.httpc.HostURL == "" {
		if err := WithHTTPHost(defaultHTTPHost)(c); err != nil {
			return nil, err
		}
	}

	if c.serviceName == "" {
		return nil, xerrors.Errorf("Service name is not specified")
	}

	if c.sendToken == "" {
		return nil, xerrors.Errorf("Send token is not specified")
	}

	if c.subscriptionToken == "" {
		return nil, xerrors.Errorf("Subscription token is not specified")
	}

	return c, nil
}

func (c *Client) GetSubscriptionSign(ctx context.Context, userID string) (xiva.SubscriptionSign, error) {
	req := c.httpc.NewRequest().
		SetHeader("Authorization", "Xiva "+c.subscriptionToken).
		SetQueryParams(map[string]string{
			"service": c.serviceName,
			"user":    userID,
		}).
		SetContext(ctx)

	var sign xiva.SubscriptionSign

	resp, err := req.Get("/v2/secret_sign")
	if err != nil {
		return sign, err
	}

	if resp.StatusCode() != http.StatusOK {
		return sign, xerrors.Errorf("failed to get secret sign data, status code: %d", resp.StatusCode())
	}

	if err := json.Unmarshal(resp.Body(), &sign); err != nil {
		return sign, xerrors.Errorf("failed to parse xiva response: %w", err)
	}

	return sign, nil
}

func (c *Client) GetWebsocketURL(ctx context.Context, userID, sessionID, clientName string, filter *xiva.Filter) (string, error) {
	sign, err := c.GetSubscriptionSign(ctx, userID)
	if err != nil {
		return "", err
	}

	queryParams := map[string]string{
		"service": c.serviceName,
		"client":  clientName,
		"session": sessionID,
		"user":    userID,
		"sign":    sign.Sign,
		"ts":      sign.Timestamp,
	}

	if filter != nil {
		filterString, err := filter.String()
		if err != nil {
			return "", err
		}
		queryParams["filter"] = filterString
	}

	values := url.Values{}
	for key, val := range queryParams {
		values.Set(key, val)
	}

	subscriptionURL := url.URL{
		Scheme:   "wss",
		Host:     c.host,
		Path:     "/v2/subscribe/websocket",
		RawQuery: values.Encode(),
	}

	return subscriptionURL.String(), nil
}

func (c *Client) SendEvent(ctx context.Context, userID, eventID string, event xiva.Event) error {
	req := c.httpc.NewRequest().
		SetHeader("Authorization", "Xiva "+c.sendToken).
		SetQueryParams(map[string]string{
			"event": eventID,
			"user":  userID,
		}).
		SetContext(ctx).
		SetBody(event)

	resp, err := req.Post("/v2/send")
	if err != nil {
		return err
	}

	if resp.IsError() {
		return xerrors.Errorf("failed to send event to xiva, status code %d (%s)", resp.StatusCode(), resp.Body())
	}

	return nil
}

func (c *Client) ListActiveSubscriptions(ctx context.Context, userID string) ([]xiva.Subscription, error) {
	req := c.httpc.NewRequest().
		SetHeader("Authorization", fmt.Sprintf("Xiva %s", c.sendToken)).
		SetQueryParams(map[string]string{
			"user":    userID,
			"service": c.serviceName,
		}).
		SetContext(ctx)
	resp, err := req.Get("/v2/list") // https://console.push.yandex-team.ru/#api-reference-list
	if err != nil {
		return nil, xerrors.Errorf("failed to send /v2/list request: %w", err)
	}
	if resp.IsError() {
		return nil, xerrors.Errorf("request /v2/list returns error response: status code %d, (%s)", resp.StatusCode(), resp.Body())
	}

	var result []xiva.Subscription
	if err := json.Unmarshal(resp.Body(), &result); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal /v2/list response: %w", err)
	}

	return result, nil
}
