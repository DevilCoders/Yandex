package kmslbclient

import (
	"time"

	"github.com/imdario/mergo"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

type ClientOptions struct {
	TokenProvider IAMTokenProvider
	Plaintext     bool
	TLSTarget     string
	Logger        log.Logger
	Retry         RetryOptions
	Balancer      BalancerOptions
}

type RetryOptions struct {
	InitialBackoff    time.Duration
	MaxBackoff        time.Duration
	BackoffMultiplier float64
	Jitter            float64
	MaxRetries        int
}

type BalancerOptions struct {
	ConnectionsPerHost int

	MaxConcurrentRequestsPerHost int

	KeepAliveTime time.Duration

	PingRefreshTime      time.Duration
	MaxPing              time.Duration
	NumPings             int
	SimilarPingThreshold time.Duration
	SimilarPingRatio     float64

	MaxTimePerTry       time.Duration
	MaxFailCountPerHost int
	FailWindowPerHost   time.Duration
	AllowPanicMode      bool
	RetryHostAfter      time.Duration

	Jitter float64
}

var defaultOptions = ClientOptions{
	Retry: RetryOptions{
		InitialBackoff:    10 * time.Millisecond,
		MaxBackoff:        100 * time.Millisecond,
		BackoffMultiplier: 1.5,
		Jitter:            0.25,
		MaxRetries:        10,
	},
	Balancer: BalancerOptions{
		ConnectionsPerHost: 1,

		MaxConcurrentRequestsPerHost: 0,

		KeepAliveTime:        10 * time.Second,
		PingRefreshTime:      600 * time.Second,
		MaxPing:              100 * time.Millisecond,
		NumPings:             10,
		SimilarPingThreshold: time.Millisecond,
		SimilarPingRatio:     1.2,

		MaxTimePerTry:       time.Second,
		MaxFailCountPerHost: 5,
		FailWindowPerHost:   5 * time.Second,
		AllowPanicMode:      true,
		RetryHostAfter:      5 * time.Second,

		Jitter: 0.25,
	},
}

func fillDefaultOptions(options *ClientOptions) *ClientOptions {
	dst := *options
	if dst.Logger == nil {
		dst.Logger = &nop.Logger{}
	}
	// mergo panics if we try to Merge the whole ClientOptions, merge the substructs
	err := mergo.Merge(&dst.Retry, defaultOptions.Retry)
	if err != nil {
		// This is a programmer error.
		panic(err)
	}
	err = mergo.Merge(&dst.Balancer, defaultOptions.Balancer)
	if err != nil {
		// This is a programmer error.
		panic(err)
	}
	return &dst
}
