package dns

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

type Checker struct {
	resolver Resolver
	host     string
	tester   func(ctx context.Context, err error) bool
}

func NewChecker(host string, opts ...Option) *Checker {
	c := &Checker{
		resolver: NewDefaultResolver(),
		host:     host,
		tester: func(_ context.Context, _ error) bool {
			// always retry
			return false
		},
	}

	for _, opt := range opts {
		c = opt(c)
	}

	return c
}

func (c *Checker) IsReady(_ context.Context) error {
	_, err := c.resolver.LookupHost(c.host)
	return err
}

func (c *Checker) EnsureDNS(ctx context.Context, timeout time.Duration, period time.Duration) error {
	return ready.WaitWithTimeout(ctx, timeout, c, ready.ErrorTesterFunc(c.tester), period)
}

type Option func(c *Checker) *Checker

func WithResolver(r Resolver) Option {
	return func(c *Checker) *Checker {
		c.resolver = r
		return c
	}
}

func WithTester(tester func(ctx context.Context, err error) bool) Option {
	return func(c *Checker) *Checker {
		c.tester = tester
		return c
	}
}
