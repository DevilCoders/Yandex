package marketplace

import (
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/library/go/core/log"
)

type Client struct {
	client *resty.Client

	logger         log.Logger
	OpTimeout      time.Duration
	OpPollInterval time.Duration
}

type Config struct {
	Endpoint       string
	OpTimeout      time.Duration
	OpPollInterval time.Duration
}

func NewClient(config Config, logger log.Logger) *Client {
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

	return &Client{
		client:         client,
		logger:         logger,
		OpTimeout:      config.OpTimeout,
		OpPollInterval: config.OpPollInterval,
	}
}
