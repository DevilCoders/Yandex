package grpccon

import (
	"context"
	"net"
	"net/url"
	"time"
)

// Dialer is interface compatible with GRPC
type Dialer interface {
	Dial(ctx context.Context, addr string) (net.Conn, error)
}

// NewDialer created dialer with some predefined defaults and options
func NewDialer(opts ...DialerOption) Dialer {
	inner := &net.Dialer{
		Timeout:       time.Second,
		DualStack:     false,
		FallbackDelay: -time.Second, // Do not fallback to IPV4 from IPV6
		KeepAlive:     10 * time.Second,
		Resolver:      &net.Resolver{PreferGo: true},
	}
	for _, o := range opts {
		o(inner)
	}

	return &dialer{
		inner: inner,
	}
}

// DialerOption is interface for Dialer interface settings customization
type DialerOption func(*net.Dialer)

// DialTimeout sets connection timetout to net.Dialer
func DialTimeout(t time.Duration) DialerOption {
	return func(d *net.Dialer) {
		d.Timeout = t
	}
}

// DialKeepAlive sets network connection keep alive
func DialKeepAlive(t time.Duration) DialerOption {
	return func(d *net.Dialer) {
		d.KeepAlive = t
	}
}

type dialer struct {
	inner *net.Dialer
}

func (d *dialer) Dial(ctx context.Context, addr string) (net.Conn, error) {
	network := "tcp"
	t, err := url.Parse(addr)
	if err != nil {
		return nil, err
	}
	if t.Scheme == "unix" {
		network = "unix"
		addr = t.Path
		if addr == "" {
			addr = t.Host
		}
	}
	return d.inner.DialContext(ctx, network, addr)
}
