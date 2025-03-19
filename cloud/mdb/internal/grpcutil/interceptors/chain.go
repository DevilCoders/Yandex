package interceptors

import (
	grpc_middleware "github.com/grpc-ecosystem/go-grpc-middleware"
	grpc_prometheus "github.com/grpc-ecosystem/go-grpc-prometheus"
	"github.com/opentracing/opentracing-go"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
)

func init() {
	grpc_prometheus.EnableClientHandlingTimeHistogram()
	grpc_prometheus.EnableHandlingTimeHistogram()
}

// ChainUnaryServerInterceptors chains all interceptors in correct order
// TODO: make backoff optional
func ChainUnaryServerInterceptors(
	backoff *retry.BackOff,
	exposeErrorDebug bool,
	readOnlyChecker ReadOnlyChecker,
	logCfg LoggingConfig,
	l log.Logger,
	more ...grpc.UnaryServerInterceptor,
) grpc.UnaryServerInterceptor {
	interceptors := []grpc.UnaryServerInterceptor{
		grpc_prometheus.UnaryServerInterceptor,
		newUnaryServerInterceptorRequestAttributes(),
		newUnaryServerInterceptorTracing(opentracing.GlobalTracer(), l),
		newUnaryServerInterceptorRequestID(),
		newUnaryServerInterceptorPanic(exposeErrorDebug, l),
		newUnaryServerInterceptorErrorToGRPC(exposeErrorDebug, l),
		newUnaryServerInterceptorSentry(),
		newUnaryServerInterceptorLogging(logCfg, l),
		newUnaryServerInterceptorReadOnly(readOnlyChecker),
		newUnaryServerInterceptorIdempotence(l),
	}

	if backoff != nil {
		interceptors = append(interceptors, newUnaryServerInterceptorRetry(backoff, l))
	}

	return grpc_middleware.ChainUnaryServer(append(interceptors, more...)...)
}

// ChainStreamServerInterceptors chains all interceptors in correct order
func ChainStreamServerInterceptors(
	exposeErrorDebug bool,
	readOnlyChecker ReadOnlyChecker,
	l log.Logger,
	more ...grpc.StreamServerInterceptor,
) grpc.StreamServerInterceptor {
	return grpc_middleware.ChainStreamServer(
		append(
			[]grpc.StreamServerInterceptor{
				grpc_prometheus.StreamServerInterceptor,
				newStreamServerInterceptorRequestAttributes(),
				newStreamServerInterceptorTracing(opentracing.GlobalTracer(), l),
				newStreamServerInterceptorRequestID(),
				newStreamServerInterceptorPanic(exposeErrorDebug, l),
				newStreamServerInterceptorErrorToGRPC(exposeErrorDebug, l),
				newStreamServerInterceptorSentry(),
				newStreamServerInterceptorLogging(l),
				newStreamServerInterceptorReadOnly(readOnlyChecker),
			},
			more...,
		)...,
	)
}

// Client interceptors

func ChainUnaryClientInterceptors(retries ClientRetryConfig, logCfg LoggingConfig, l log.Logger) grpc.UnaryClientInterceptor {
	return grpc_middleware.ChainUnaryClient(
		grpc_prometheus.UnaryClientInterceptor,
		newUnaryClientInterceptorTracing(opentracing.GlobalTracer()),
		newUnaryClientInterceptorIdempotence(),
		newUnaryClientInterceptorRetry(retries),
		newUnaryClientInterceptorRequestID(),
		newUnaryClientInterceptorLogging(logCfg, l),
	)
}

func ChainStreamClientInterceptors(l log.Logger) grpc.StreamClientInterceptor {
	return grpc_middleware.ChainStreamClient(
		// TODO: add missing interceptors
		grpc_prometheus.StreamClientInterceptor,
		newStreamClientInterceptorTracing(opentracing.GlobalTracer()),
		newStreamClientInterceptorLogging(l),
	)
}
