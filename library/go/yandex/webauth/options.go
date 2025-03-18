package webauth

import (
	"net/url"

	"github.com/jonboulle/clockwork"
)

type ClientOption func(c *HTTPClient)

func WithBaseURL(baseURL url.URL) ClientOption {
	return func(c *HTTPClient) {
		c.baseURL = baseURL
	}
}

func WithClock(clock clockwork.Clock) ClientOption {
	return func(c *HTTPClient) {
		c.storage.clock = clock
	}
}
