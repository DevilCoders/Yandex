package billing

import (
	"net/http"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
)

const (
	retryMinWaitTime = 100 * time.Millisecond
	retryMaxWaitTime = 10 * time.Second
)

type Client struct {
	client *resty.Client

	logger log.Logger
}

type Config struct {
	Endpoint string

	RetryCount int
}

func NewClient(config Config, logger log.Logger) *Client {

	logger.Debug("running billing client", log.String("endpoint", config.Endpoint))

	client := resty.New().
		SetBaseURL(config.Endpoint).
		OnBeforeRequest(
			func(c *resty.Client, request *resty.Request) error {
				if span := opentracing.SpanFromContext(request.Context()); span != nil {
					_ = tracing.Tracer().Inject(
						span.Context(),
						opentracing.HTTPHeaders,
						opentracing.HTTPHeadersCarrier(request.Header),
					)
				}

				return nil
			},
		)

	if config.RetryCount != 0 {
		client = client.
			SetRetryCount(config.RetryCount).
			SetRetryWaitTime(retryMinWaitTime).
			SetRetryMaxWaitTime(retryMaxWaitTime).
			AddRetryCondition(func(r *resty.Response, _ error) bool {
				switch r.StatusCode() {
				case http.StatusGatewayTimeout:
					logger.Warn("retrying after gateway timeout")
					return true
				case http.StatusTooManyRequests:
					logger.Warn("retrying after too many requests status")
					return true
				default:
					logger.Debug("billing client: no conditions for retry")
					return false
				}
			})

		logger.Info("attached backoff retry mechanics to billing private api client", log.Int("retry_count", config.RetryCount))
	}

	return &Client{
		client: client,
		logger: logger,
	}
}
