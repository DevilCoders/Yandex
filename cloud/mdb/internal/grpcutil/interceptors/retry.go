package interceptors

import (
	"context"
	"time"

	grpc_retry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
)

func newUnaryServerInterceptorRetry(backoff *retry.BackOff, l log.Logger) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		var resp interface{}
		err := backoff.RetryWithLog(
			ctx,
			func() error {
				var attemptErr error
				resp, attemptErr = handler(ctx, req)
				if attemptErr == nil {
					return nil
				}

				errCode := codes.Unknown
				if sem := semerr.AsSemanticError(attemptErr); sem != nil {
					errCode = grpcerr.SemanticErrorToGRPC(sem.Semantic)
				}

				if !grpcerr.IsTemporary(errCode) {
					attemptErr = retry.Permanent(attemptErr)
				}

				return attemptErr
			},
			"gRPC ingress",
			l,
		)

		return resp, err
	}
}

type ClientRetryConfig struct {
	// MaxTries 0 (retries disabled) equals one initial try
	MaxTries uint `yaml:"max_tries"`
	// PerRetryTimeout determines timeout of each try itself
	PerRetryTimeout time.Duration `yaml:"per_retry_timeout"`
}

func DefaultClientRetryConfig() ClientRetryConfig {
	return ClientRetryConfig{
		MaxTries:        3,
		PerRetryTimeout: time.Second,
	}
}

// newUnaryClientInterceptorRetry returns interceptor for request retries
func newUnaryClientInterceptorRetry(cfg ClientRetryConfig) grpc.UnaryClientInterceptor {
	// TODO: exponential backoff
	return grpc_retry.UnaryClientInterceptor(
		grpc_retry.WithMax(cfg.MaxTries),
		grpc_retry.WithPerRetryTimeout(cfg.PerRetryTimeout),
	)
}
