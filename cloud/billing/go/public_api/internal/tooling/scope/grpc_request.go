package scope

import (
	"context"
	"time"

	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/clock"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/logging"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/metrics"
	requestid "a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/request_id"
	"a.yandex-team.ru/library/go/core/log"
)

var _ handlerScope = &grpcRequest{}

type grpcRequest struct {
	logger    log.Logger
	requestID string

	StartedAt time.Time
	Method    string
}

func (g *grpcRequest) Logger() log.Logger {
	return g.logger
}

func (g *grpcRequest) RequestID() string {
	return g.requestID
}

func StartGRPCRequest(ctx context.Context, method string) context.Context {
	parentScope := getCurrent(ctx)
	requestID := requestid.ExtractOrGenerateGRPC(ctx, parentScope.Logger())
	fields := []log.Field{
		logging.GRPCFullMethod(method),
		logging.RequestID(requestID),
	}

	logger := log.With(parentScope.Logger(), fields...)

	grpcScope := &grpcRequest{
		logger:    logger,
		requestID: requestID,
		StartedAt: clock.Get().Now(),
		Method:    method,
	}
	ctx = newScopeContext(grpcScope, ctx)

	grpcScope.logger.Info("start unary grpc call")
	metrics.GRPCRequestStarted.WithLabelValues(metrics.GRPCRequestStartedLabels(method)...).Inc()

	return ctx
}

func FinishGRPCRequest(ctx context.Context, err error) {
	grpcScope := getCurrent(ctx).(*grpcRequest)
	injectErr := requestid.InjectGRPC(ctx, grpcScope.RequestID())
	if injectErr != nil {
		grpcScope.Logger().Error("cant inject request_id to outgoing metadata")
	}

	duration := clock.Get().Since(grpcScope.StartedAt)

	code := status.Code(err)
	fields := []log.Field{
		logging.DurationMS(duration),
		logging.GRPCCode(code),
	}

	if err == nil {
		grpcScope.logger.Info("success unary grpc call", fields...)
	} else {
		fields = append(fields, log.Error(err))
		grpcScope.logger.Error("error unary grpc call", fields...)
	}
	metrics.GRPCRequestFinished.WithLabelValues(metrics.GRPCRequestFinishedLabels(grpcScope.Method, code)...).Inc()
	metrics.GRPCRequestDuration.WithLabelValues(
		metrics.GRPCRequestDurationLabels(grpcScope.Method, code)...,
	).Observe(float64(duration.Milliseconds()))
}
