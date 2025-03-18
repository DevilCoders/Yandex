package httplaas

import (
	"errors"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/library/go/core/log"
)

// HTTPClientOpt is a function that sets client behavior or basic data.
// Example:
//    c, err := NewHTTPClient(
//        AsService("alice"),
//        WithAppUUID(uuid.Must(uuid.FromString("c06e5a28-0bdf-4d4e-9dd0-86a5a51e9a0d"))),
//        WithHTTPHost("http://mock-laas.yandex-team.ru"),
//    )
type ClientOpt func(c *Client) error

// AsService supplies service name for http client for debugging.
func AsService(name string) ClientOpt {
	return func(c *Client) error {
		if name == "" {
			return errors.New("laas: service name cannot be empty")
		}
		c.httpc.SetQueryParam("service", name)
		return nil
	}
}

// WithAppUUID supplies unique installed app UUID for http client.
func WithAppUUID(u uuid.UUID) ClientOpt {
	return func(c *Client) error {
		c.httpc.SetQueryParam("uuid", u.String())
		return nil
	}
}

// WithHTTPHost rewrites default HTTP host in client.
func WithHTTPHost(host string) ClientOpt {
	return func(c *Client) error {
		c.httpc.SetBaseURL(host)
		return nil
	}
}

// WithDebug enables debug resty output
func WithDebug(enable bool) ClientOpt {
	return func(c *Client) error {
		c.httpc.SetDebug(enable)
		return nil
	}
}

// WithLogger redirects default resty debug output to custom logger.
func WithLogger(l log.Logger) ClientOpt {
	return func(c *Client) error {
		c.httpc.SetLogger(l)
		return nil
	}
}
