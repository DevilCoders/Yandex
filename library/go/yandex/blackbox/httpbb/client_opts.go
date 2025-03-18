package httpbb

import (
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Option func(c *Client)

// WithLogger sets logger to use.
func WithLogger(l log.Structured) Option {
	return func(c *Client) {
		c.http.setLogger(l)
	}
}

// WithTVM sets TVM client to use. If you requested passport grants for TVM application - you must set it.
func WithTVM(tvmClient tvm.Client) Option {
	return func(c *Client) {
		c.http.tvmc = tvmClient
	}
}

// WithRetries sets retries count.
// Default: 3
func WithRetries(retries int) Option {
	return func(c *Client) {
		c.http.retries = uint64(retries)
	}
}

// WithTimeout sets timeout of single request.
// Default: 200ms
func WithTimeout(timeout time.Duration) Option {
	return func(c *Client) {
		c.http.setTimeout(timeout)
	}
}

// WithDebug enables the debug mode on Resty client.
// Default: false
func WithDebug(debug bool) Option {
	return func(c *Client) {
		c.http.setDebug(debug)
	}
}
