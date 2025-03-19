package unifiedagent

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"

	"github.com/go-resty/resty/v2"
)

type SolomonMetric struct {
	Labels    map[string]string `json:"labels"`
	Value     json.Number       `json:"value"`
	Timestamp int64             `json:"ts"`
}

var (
	ErrPostMetrics = errsentinel.New("post metrics")
	ErrHealthCheck = errsentinel.New("health check")
)

const (
	requestTimeout = time.Second * 5
)

type uaClient struct {
	solomonMetricsPort uint
	healthCheckPort    uint
	httpClient         *resty.Client
}

var _ UAClient = (*uaClient)(nil)

func NewUAClient(solomonMetricsPort uint, healthCheckPort uint) UAClient {
	return &uaClient{
		solomonMetricsPort: solomonMetricsPort,
		healthCheckPort:    healthCheckPort,
		httpClient:         resty.New().SetTimeout(requestTimeout),
	}
}

func (c *uaClient) PushMetrics(ctx context.Context, metrics ...SolomonMetric) error {
	var payload struct {
		Metrics []SolomonMetric `json:"metrics"`
	}
	payload.Metrics = metrics

	resp, err := c.httpClient.R().
		SetContext(ctx).
		SetBody(payload).
		Post(fmt.Sprintf("http://localhost:%d/write", c.solomonMetricsPort))
	if err != nil {
		return ErrPostMetrics.Wrap(err)
	}

	if resp.StatusCode() != http.StatusOK {
		return fmt.Errorf("response status %d", resp.StatusCode())
	}

	return nil
}

func (c *uaClient) HealthCheck(ctx context.Context) error {
	_, err := c.httpClient.R().
		SetContext(ctx).
		Get(fmt.Sprintf("http://localhost:%d/status", c.healthCheckPort))
	if err != nil {
		return ErrHealthCheck.Wrap(err)
	}

	return nil
}
