package dial

import (
	"net"
	"time"
)

type Config struct {
	ConnectTimeout  time.Duration `json:"connect_timeout" yaml:"connect_timeout"`
	KeepAlivePeriod time.Duration `json:"keep_alive_period" yaml:"keep_alive_period"`
}

func DefaultConfig() Config {
	return Config{
		ConnectTimeout:  time.Second,
		KeepAlivePeriod: time.Second * 5,
	}
}

func (c Config) Dialer() *net.Dialer {
	return &net.Dialer{
		Timeout:   c.ConnectTimeout,
		KeepAlive: c.KeepAlivePeriod,
	}
}

func (c Config) Resolver() *net.Resolver {
	dialer := c.Dialer()
	return &net.Resolver{
		Dial: dialer.DialContext,
	}
}

type TCPConfig struct {
	Connect   Config `json:"connect" yaml:"connect"`
	DNSLookup Config `json:"dns_lookup" yaml:"dns_lookup"`
}

func DefaultTCPConfig() TCPConfig {
	return TCPConfig{
		Connect:   DefaultConfig(),
		DNSLookup: DefaultConfig(),
	}
}
