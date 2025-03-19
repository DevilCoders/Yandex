package connection

import (
	"time"

	"golang.org/x/xerrors"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/keepalive"

	"github.com/opentracing/opentracing-go"

	grpcretry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
	otgrpc "github.com/opentracing-contrib/go-grpc"
)

const (
	defaultMarketplaceUserAgent = "YC/Marketplace"
)

type connConfig struct {
	endpoint string

	caPath    string
	userAgent string

	initTimeout time.Duration

	maxRetries   uint
	retryTimeout time.Duration

	keepAliveTime    time.Duration
	keepAliveTimeout time.Duration

	debugMode bool

	authenticator Authenticator

	tracer opentracing.Tracer
}

var defaultConfig = connConfig{
	endpoint:  "",
	caPath:    "",
	userAgent: defaultMarketplaceUserAgent,

	initTimeout:  20 * time.Second,
	maxRetries:   3,
	retryTimeout: 2 * time.Second,

	keepAliveTime:    20 * time.Second,
	keepAliveTimeout: 2 * time.Second,
}

type Option interface {
	apply(*connConfig)
}

type funcConnOptions struct {
	f func(*connConfig)
}

func (fdo *funcConnOptions) apply(do *connConfig) {
	fdo.f(do)
}

func newFuncConnOption(f func(*connConfig)) *funcConnOptions {
	return &funcConnOptions{
		f: f,
	}
}

func WithInitTimeout(timeout time.Duration) Option {
	return newFuncConnOption(func(c *connConfig) {
		c.initTimeout = timeout
	})
}

func WithRetries(retries int, timeout time.Duration) Option {
	return newFuncConnOption(func(c *connConfig) {
		c.maxRetries = uint(retries)
		c.retryTimeout = timeout
	})
}

func WithKeepAliveTimeout(timeout time.Duration) Option {
	return newFuncConnOption(func(c *connConfig) {
		c.keepAliveTimeout = timeout
	})
}

func WithKeepAliveTime(timeout time.Duration) Option {
	return newFuncConnOption(func(c *connConfig) {
		c.keepAliveTime = timeout
	})
}

func WithTLS(caPath string) Option {
	return newFuncConnOption(func(c *connConfig) {
		c.caPath = caPath
	})
}

func WithPerRPCAuth(authenticator Authenticator) Option {
	return newFuncConnOption(func(c *connConfig) {
		c.authenticator = authenticator
	})
}

func WithDebugMode() Option {
	return newFuncConnOption(func(c *connConfig) {
		c.debugMode = true
	})
}

func WithTracer(tracer opentracing.Tracer) Option {
	return newFuncConnOption(func(c *connConfig) {
		c.tracer = tracer
	})
}

func (c connConfig) generateDialOptions() (out []grpc.DialOption, err error) {
	out = append(out,
		grpc.WithBlock(),
		grpc.WithUserAgent(c.userAgent),
	)

	if c.tracer != nil {
		out = append(out,
			grpc.WithChainUnaryInterceptor(
				otgrpc.OpenTracingClientInterceptor(c.tracer),
			),
		)
	}

	if c.keepAliveTime > 0 {
		out = append(out,
			grpc.WithKeepaliveParams(keepalive.ClientParameters{
				Time:    c.keepAliveTime,
				Timeout: c.keepAliveTimeout,

				PermitWithoutStream: true,
			}),
		)
	}

	if c.retryTimeout > 0 {
		retryInterceptor := grpcretry.UnaryClientInterceptor(
			grpcretry.WithMax(c.maxRetries),
			grpcretry.WithPerRetryTimeout(c.retryTimeout),
		)

		out = append(out,
			grpc.WithChainUnaryInterceptor(
				retryInterceptor,
			),
		)
	}

	switch {
	case c.caPath != "":
		var tls credentials.TransportCredentials

		tls, err = credentials.NewClientTLSFromFile(c.caPath, "")
		if err != nil {
			return nil, err
		}

		out = append(out,
			grpc.WithTransportCredentials(tls),
		)

		if c.authenticator != nil {
			out = append(out, grpc.WithPerRPCCredentials(
				newTokenBaseAuthenticator(c.authenticator),
			))
		}
	default:
		out = append(out,
			grpc.WithTransportCredentials(
				insecure.NewCredentials(),
			),
		)
	}

	return
}

func (c connConfig) validate() error {
	if c.endpoint == "" {
		return xerrors.New("grpc connection endpoint must be provided")
	}

	if !c.debugMode && c.caPath == "" && c.authenticator != nil {
		return xerrors.New("tls path should be provided if credentials are set")
	}

	return nil
}
