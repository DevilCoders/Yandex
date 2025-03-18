package webauth

import (
	"context"
	"fmt"
	"net/http"
	"net/url"

	"github.com/go-resty/resty/v2"
)

type Client interface {
	CheckRoleBySessionID(ctx context.Context, sessionID, role string) (string, bool, error)
	CheckRoleByToken(ctx context.Context, token, role string) (string, bool, error)

	GetLoginBySessionID(ctx context.Context, sessionID string) (string, error)
	GetLoginByToken(ctx context.Context, token string) (string, error)
}

type HTTPClient struct {
	baseURL url.URL
	r       *resty.Client
	storage *AuthStorage
}

func (c *HTTPClient) makeRequest(ctx context.Context, credentials string, role string, asToken bool) (*resty.Response, error) {
	ref, err := url.Parse("/auth_request")
	if err != nil {
		return nil, fmt.Errorf("webauth: failed tp construct request: %w", err)
	}

	request := c.r.R()
	request.SetContext(ctx)

	if asToken {
		request.SetHeader("Authorization", fmt.Sprintf("OAuth %s", credentials))
	} else {
		request.SetCookie(&http.Cookie{Name: "Session_id", Value: credentials})
	}

	if role != "" {
		request.QueryParam.Add("idm_role", role)
	}

	response, err := request.Execute("GET", c.baseURL.ResolveReference(ref).String())
	if err != nil {
		return nil, err
	}

	return response, err
}

func (c *HTTPClient) GetLoginByToken(ctx context.Context, token string) (string, error) {
	return c.getLoginWithContext(ctx, token, true)
}

func (c *HTTPClient) GetLoginBySessionID(ctx context.Context, sessionID string) (string, error) {
	return c.getLoginWithContext(ctx, sessionID, false)
}

func (c *HTTPClient) getLoginWithContext(ctx context.Context, credentials string, asToken bool) (string, error) {
	if credentials == "" {
		return "", fmt.Errorf("webauth: missing credentials")
	}

	login, ok := c.storage.GetLogin(credentials, asToken)
	if ok {
		return login, nil
	}

	response, err := c.makeRequest(ctx, credentials, "", asToken)
	if err != nil {
		return "", fmt.Errorf("webauth: request failed: %w", err)
	}
	login = response.Header().Get("X-Webauth-Login")

	if login == "" {
		return login, fmt.Errorf("webauth: not authenticated")
	}

	c.storage.SetLogin(credentials, login, asToken)
	return login, nil
}

func (c *HTTPClient) CheckRoleByToken(ctx context.Context, token, role string) (string, bool, error) {
	return c.checkRoleWithContext(ctx, token, role, true)
}

func (c *HTTPClient) CheckRoleBySessionID(ctx context.Context, sessionID, role string) (string, bool, error) {
	return c.checkRoleWithContext(ctx, sessionID, role, false)
}

func (c *HTTPClient) checkRoleWithContext(ctx context.Context, credentials, role string, asToken bool) (string, bool, error) {
	if credentials == "" {
		return "", false, fmt.Errorf("webauth: missing credentials")
	}

	login, granted, ok := c.storage.GetRole(credentials, role, asToken)
	if ok {
		return login, granted, nil
	}

	response, err := c.makeRequest(ctx, credentials, role, asToken)
	if err != nil {
		return "", false, fmt.Errorf("webauth: request failed: %w", err)
	}
	login = response.Header().Get("X-Webauth-Login")

	if response.StatusCode() == http.StatusForbidden {
		c.storage.SetRole(credentials, login, role, false, asToken)
		return login, false, nil
	}
	if response.StatusCode() == http.StatusOK {
		c.storage.SetRole(credentials, login, role, true, asToken)
		return login, true, nil
	}

	return "", false, fmt.Errorf("webauth: unexpected HTTP status: %v", response.StatusCode())
}

func NewWebauthClient(opts ...ClientOption) *HTTPClient {
	r := resty.New()
	r.SetRetryCount(3)

	client := &HTTPClient{
		r: r,
		baseURL: url.URL{
			Scheme: "https",
			Host:   "webauth.yandex-team.ru",
		},
		storage: newAuthStorage(),
	}

	for _, opt := range opts {
		opt(client)
	}

	return client
}
