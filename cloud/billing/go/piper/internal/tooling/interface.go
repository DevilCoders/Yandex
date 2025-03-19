package tooling

import (
	"context"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/features"
	"a.yandex-team.ru/library/go/core/log"
)

// Action traces
func ActionStarted(ctx context.Context) context.Context {
	caller := getCallFuncName(1)
	return ActionWithNameStarted(ctx, caller)
}

func ActionWithNameStarted(ctx context.Context, name string) context.Context {
	return actionStarted(ctx, name)
}

func ActionDone(ctx context.Context, err error) {
	actionDone(ctx, err)
}

func IncomingMetricsCount(ctx context.Context, size int) {
	incomingMetricsCount(ctx, size)
}

func InvalidMetrics(ctx context.Context, metrics []entities.InvalidMetric) {
	invalidMetrics(ctx, metrics)
}

func ProcessedMetricsCount(ctx context.Context, size int) {
	processedMetricsCount(ctx, size)
}

func MetricSchemaUsageLagObserver(ctx context.Context, schema string) IntObserver {
	return getProcessLagObserverForSchema(ctx, schema)
}

func MetricSchemaWriteLagObserver(ctx context.Context, schema string) IntObserver {
	return getWriteLagObserverForSchema(ctx, schema)
}

func CumulativeDiffSize(ctx context.Context, size int) {
	cumulativeDiffSize(ctx, size)
}

// DB traces
func QueryWithRowsCount(ctx context.Context, queryRowsCount int) context.Context {
	return dbQueryRowsCount(ctx, queryRowsCount)
}
func QueryStarted(ctx context.Context) context.Context {
	caller := getCallFuncName(1)
	return QueryWithNameStarted(ctx, caller)
}

// DB traces
func QueryWithNameStarted(ctx context.Context, queryName string) context.Context {
	return dbQueryStarted(ctx, queryName)
}

func QueryDone(ctx context.Context, err error) {
	dbQueryDone(ctx, err)
}

// Interconnect traces
func ICRequestStarted(ctx context.Context, system string) context.Context {
	caller := getCallFuncName(1)
	return ICRequestWithNameStarted(ctx, system, caller)
}

func ICRequestWithNameStarted(ctx context.Context, system, requestName string) context.Context {
	return icRequestStarted(ctx, system, requestName)
}

func ICRequestDone(ctx context.Context, err error) {
	icRequestDone(ctx, err)
}

// Retry check helpers
func StartRetry(ctx context.Context) context.Context {
	return startRetry(ctx)
}

func RetryIteration(ctx context.Context) {
	retryIteration(ctx)
}

// Logging
func Logger(ctx context.Context) log.Structured {
	return logger(ctx)
}

func Debug(ctx context.Context, msg string, fields ...log.Field) {
	Logger(ctx).Debug(msg, fields...)
}

// Tracing
func TraceTag(ctx context.Context, tags ...opentracing.Tag) {
	traceTag(ctx, tags)
}

func TraceEvent(ctx context.Context, event string, fields ...log.Field) {
	traceEvent(ctx, event, fields)
}

// Features
func Features(ctx context.Context) features.Flags {
	return featuresFlags(ctx)
}

func ExposeRequestID(ctx context.Context) string {
	return exposeRequestID(ctx)
}

func LocalTime(ctx context.Context, t time.Time) time.Time {
	f := featuresFlags(ctx)
	return t.In(f.LocalTimezone())
}
