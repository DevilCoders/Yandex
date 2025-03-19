package console

import (
	"context"
	"net"
	"net/http"
	"time"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/scope"
)

type consoleClient struct {
	client *resty.Client
}

func NewClient(hostURL string) Client {
	client := resty.NewWithClient(httpClient()).
		SetHostURL(hostURL).
		SetRetryCount(5).
		SetRetryWaitTime(500*time.Millisecond).
		SetHeader("User-Agent", "billing-public-api")

	client.OnBeforeRequest(scope.StartHTTPCall)
	client.OnAfterResponse(scope.FinishHTTPCallResponse)
	client.OnError(scope.FinishHTTPCallErr)

	return &consoleClient{
		client,
	}
}

const (
	httpMaxIdleConns    = 20
	httpMaxConnsPerHost = 20
	httpIdleHostTimeout = time.Minute
)

func httpClient() *http.Client {
	dialer := &net.Dialer{
		Timeout:  time.Second, // Die fast. If network is dead, then fail.
		Resolver: &net.Resolver{PreferGo: true},
	}

	return &http.Client{
		Transport: &http.Transport{
			MaxIdleConns:    httpMaxIdleConns,
			MaxConnsPerHost: httpMaxConnsPerHost,
			IdleConnTimeout: httpIdleHostTimeout,
			DialContext:     dialer.DialContext,
		},
		Timeout: time.Minute, // Max value in our metrics :)
	}
}

func (c *consoleClient) createRequest(ctx context.Context, auth *AuthData) *resty.Request {
	request := c.client.R()
	request.SetContext(ctx)
	request.SetHeader(authTokenHeaderName, auth.Token)

	return request
}
