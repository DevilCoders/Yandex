package tooling

import (
	"context"
	"fmt"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/features"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/metrics"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracing"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func actionStarted(ctx context.Context, action string) context.Context {
	ctx, storage := deriveStore(ctx)
	storage.action = action

	storage.span = tracing.ChildSpan(
		action, storage.span, tracetag.ActionSpan(), tracetag.Action(action),
	)
	metrics.ActionsStarted.WithLabelValues(storage.actionStarLabels()...).Inc()
	storage.logger().Debug("start action")
	return ctx
}

func actionDone(ctx context.Context, err error) {
	storage := getNotEmptyStoreFromCtx(ctx)

	if err != nil {
		tracing.TagSpan(storage.span, tracetag.Failed())
		tracing.SpanEvent(storage.span, "action failed", logf.Error(err))
	}
	tracing.FinishSpan(storage.span)

	metrics.ActionsDone.WithLabelValues(storage.actionDoneLabels(err == nil)...).Inc()
	if err == nil {
		storage.logger().Debug("action done")
		return
	}
	storage.logger().Error("action failed", logf.Error(err))
}

func incomingMetricsCount(ctx context.Context, size int) {
	storage := getNotEmptyStoreFromCtx(ctx)

	tracing.TagSpan(storage.span, tracetag.IncomingMetricsCount(size))
	logger := storage.logger()
	metrics.IncomingMetrics.WithLabelValues(storage.incomingMetricsLabels()...).Add(float64(size))
	logger.Info("incoming metrics", logf.Size(size))
}

func invalidMetrics(ctx context.Context, im []entities.InvalidMetric) {
	if len(im) == 0 {
		return
	}
	storage := getNotEmptyStoreFromCtx(ctx)

	cntByReason := map[string]int{}
	for _, m := range im {
		cntByReason[m.Reason.String()]++
	}

	tracing.TagSpan(storage.span, tracetag.InvalidMetricsCount(len(im)))

	logger := storage.logger()
	for reason, count := range cntByReason {
		metrics.InvalidMetrics.WithLabelValues(storage.invalidMetricsLabels(reason)...).Add(float64(count))
		logger.Error("invalid metrics", logf.ErrorKey("reason"), logf.ErrorValue(reason), logf.Size(count))
	}
}

func processedMetricsCount(ctx context.Context, size int) {
	storage := getNotEmptyStoreFromCtx(ctx)

	tracing.TagSpan(storage.span, tracetag.ProcessedMetricsCount(size))
	logger := storage.logger()
	metrics.ProcessedMetrics.WithLabelValues(storage.processedMetricsLabels()...).Add(float64(size))
	logger.Info("processed metrics", logf.Size(size))
}

func getProcessLagObserverForSchema(ctx context.Context, schema string) observerWrapper {
	storage := getNotEmptyStoreFromCtx(ctx)

	hst := metrics.ProducerProcessLag.WithLabelValues(storage.schemaLagLabels(schema)...)
	return observerWrapper{hst}
}

func getWriteLagObserverForSchema(ctx context.Context, schema string) observerWrapper {
	storage := getNotEmptyStoreFromCtx(ctx)

	hst := metrics.ProducerWriteLag.WithLabelValues(storage.schemaLagLabels(schema)...)
	return observerWrapper{hst}
}

func cumulativeDiffSize(ctx context.Context, size int) {
	storage := getNotEmptyStoreFromCtx(ctx)

	metrics.CumulativeChanges.WithLabelValues(storage.cumulativeChangesLabels()...).Add(float64(size))
}

func dbQueryRowsCount(ctx context.Context, queryRowsCount int) context.Context {
	ctx, storage := deriveStore(ctx)
	storage.dbQueryRowsCount = queryRowsCount
	return ctx
}

func dbQueryStarted(ctx context.Context, queryName string) context.Context {
	ctx, storage := deriveStore(ctx)

	storage.dbQueryName = queryName
	storage.dbQueryCallTime = storage.getClock().Now()

	storage.span = tracing.ChildSpan(
		queryName, storage.span, tracetag.QuerySpan(), tracetag.QueryName(queryName),
	)
	if storage.retryAttempt > 0 {
		tracing.TagSpan(storage.span, tracetag.RetryAttempt(storage.retryAttempt))
	}
	metrics.QueryStarted.WithLabelValues(storage.queryStartLabels()...).Inc()
	storage.logger().Debug("db query started")
	return ctx
}

func dbQueryDone(ctx context.Context, err error) {
	storage := getNotEmptyStoreFromCtx(ctx)
	if storage.dbQueryName == "" {
		return
	}
	if storage.dbQueryRowsCount > 0 {
		tracing.TagSpan(storage.span, tracetag.QueryRowsCount(storage.dbQueryRowsCount))
	}
	if err != nil {
		tracing.TagSpan(storage.span, tracetag.Failed())
		tracing.SpanEvent(storage.span, "query failed", logf.Error(err))
	}
	tracing.FinishSpan(storage.span)

	labels := storage.queryDoneLabels(err == nil)

	metrics.QueryDuration.WithLabelValues(labels...).Observe(storage.queryDurationMicroseconds())

	if err == nil {
		storage.logger().Debug("query done")
		return
	}
	storage.logger().Error("query failed", logf.Error(err))
}

func icRequestStarted(ctx context.Context, system, requestName string) context.Context {
	ctx, storage := deriveStore(ctx)

	storage.icSystem = system
	storage.icRequest = requestName
	storage.icRequestCallTime = storage.getClock().Now()

	storage.span = tracing.ChildSpan(
		fmt.Sprintf("%s.%s", system, requestName), storage.span,
		tracetag.InterconnectSpan(), tracetag.Service(system), tracetag.RequestName(requestName),
	)
	if storage.retryAttempt > 0 {
		tracing.TagSpan(storage.span, tracetag.RetryAttempt(storage.retryAttempt))
	}

	metrics.InterconnectStarted.WithLabelValues(storage.icStartLabels()...).Inc()
	storage.logger().Debug("request started")
	return ctx
}

func icRequestDone(ctx context.Context, err error) {
	storage := getNotEmptyStoreFromCtx(ctx)
	if storage.icRequest == "" {
		return
	}

	if err != nil {
		tracing.TagSpan(storage.span, tracetag.Failed())
		tracing.SpanEvent(storage.span, "request failed", logf.Error(err))
	}
	tracing.FinishSpan(storage.span)

	labels := storage.icDoneLabels(err == nil)

	metrics.InterconnectDuration.WithLabelValues(labels...).Observe(storage.icDurationMicroseconds())

	if err == nil {
		storage.logger().Debug("request done")
		return
	}
	storage.logger().Error("request failed", logf.Error(err))
}

func startRetry(ctx context.Context) context.Context {
	ctx, storage := deriveStore(ctx)

	storage.retryStarted = true
	return ctx
}

func retryIteration(ctx context.Context) {
	storage := getNotEmptyStoreFromCtx(ctx)
	if !storage.retryStarted {
		return
	}

	if storage.retryAttempt > 0 {
		storage.logger().Info("attempt to retry")
	}
	storage.retryAttempt++
}

func logger(ctx context.Context) log.Structured {
	storage := getStoreFromCtx(ctx)
	if storage == nil {
		return &nop.Logger{}
	}
	return storage.logger()
}

func traceTag(ctx context.Context, tags []opentracing.Tag) {
	storage := getNotEmptyStoreFromCtx(ctx)
	if storage.span == nil {
		return
	}
	tracing.TagSpan(storage.span, tags...)
}

func traceEvent(ctx context.Context, event string, fields []log.Field) {
	storage := getNotEmptyStoreFromCtx(ctx)
	storage.logger().Info(event, fields...)
	if storage.span == nil {
		return
	}
	tracing.SpanEvent(storage.span, event, fields...)
}

func featuresFlags(ctx context.Context) features.Flags {
	storage := getNotEmptyStoreFromCtx(ctx)
	if storage.flagsUpdated {
		return storage.flags
	}
	return features.Default()
}

func exposeRequestID(ctx context.Context) string {
	storage := getNotEmptyStoreFromCtx(ctx)
	return storage.requestID
}
