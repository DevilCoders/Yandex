package grpccon

import (
	"context"
	"fmt"
	"time"

	grpcretry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func Connect(
	ctx context.Context, endpoint string, config Config, auth credentials.PerRPCCredentials,
) (conn grpc.ClientConnInterface, err error) {
	config = extendConfigWithDefaults(config)

	defer func() {
		if err != nil {
			err = fmt.Errorf("can't connect to service: %w", err)
		}
	}()

	var tls credentials.TransportCredentials
	if config.TLS != nil {
		tls = credentials.NewTLS(config.TLS)
	}

	interceptorsOption := grpcUnaryInterceptorsOption(config)
	userAgentOption := grpc.WithUserAgent("Cloud Billing")
	keepAliveOption := grpcKeepAliveOption(config)
	dialerOption := grpcDialer()

	return makeGRPCConn(ctx, endpoint, tls, auth,
		interceptorsOption,
		userAgentOption,
		keepAliveOption,
		dialerOption,
	)
}

func grpcUnaryInterceptorsOption(config Config) grpc.DialOption {
	retryConfig := config.RetryConfig

	retryInterceptor := grpcretry.UnaryClientInterceptor(
		grpcretry.WithMax(retryConfig.MaxRetries),
		grpcretry.WithPerRetryTimeout(retryConfig.PerRetryTimeout),
	)
	toolingInterceptor := tooling.NewToolingUnaryInterceptor()

	return grpc.WithChainUnaryInterceptor(
		retryInterceptor,
		toolingInterceptor,
	)
}

func grpcKeepAliveOption(config Config) grpc.DialOption {
	return grpc.WithKeepaliveParams(keepalive.ClientParameters{
		Time:                config.KeepAliveConfig.Time,
		Timeout:             config.KeepAliveConfig.Timeout,
		PermitWithoutStream: true,
	})
}

func grpcDialer() grpc.DialOption {
	dialer := NewDialer()
	return grpc.WithContextDialer(dialer.Dial)
}

func makeGRPCConn(
	ctx context.Context, endpoint string, tls credentials.TransportCredentials, auth credentials.PerRPCCredentials,
	options ...grpc.DialOption,
) (conn *grpc.ClientConn, err error) {
	opt := []grpc.DialOption{}
	if tls != nil {
		opt = append(opt, grpc.WithTransportCredentials(tls))
	} else {
		opt = append(opt, grpc.WithInsecure())
	}
	opt = append(opt, grpc.WithBlock())

	if auth != nil {
		opt = append(opt, grpc.WithPerRPCCredentials(auth))
	}
	opt = append(opt, options...)

	ctx, cancel := context.WithTimeout(ctx, time.Second*10)
	defer cancel()

	conn, err = grpc.DialContext(ctx, endpoint, opt...)
	return
}
