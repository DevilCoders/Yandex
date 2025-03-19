package scope

import (
	"time"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/clock"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/logging"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/metrics"
	requestid "a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/request_id"
	"a.yandex-team.ru/library/go/core/log"
)

var _ handlerScope = &httpCall{}

type httpCall struct {
	logger    log.Logger
	requestID string

	StartedAt time.Time
}

func (h *httpCall) Logger() log.Logger {
	return h.logger
}

func (h httpCall) RequestID() string {
	return h.requestID
}

func StartHTTPCall(client *resty.Client, request *resty.Request) error {
	ctx := request.Context()

	fields := []log.Field{
		logging.HTTPMethod(request.Method),
		logging.HTTPUrlPattern(request.URL),
	}
	parentScope := getCurrent(ctx).(handlerScope)
	logger := log.With(parentScope.Logger(), fields...)

	httpScope := &httpCall{
		logger:    logger,
		requestID: parentScope.RequestID(),
		StartedAt: clock.Get().Now(),
	}
	ctx = newScopeContext(httpScope, ctx)

	httpScope.logger.Info("start console call")
	metrics.HTTPCallStarted.WithLabelValues(metrics.HTTPCallStartedLabels(request.Method, request.URL)...).Inc()

	requestid.InjectHTTP(httpScope.requestID, request)
	request.SetContext(ctx)

	return nil
}

func FinishHTTPCallResponse(client *resty.Client, response *resty.Response) error {
	ctx := response.Request.Context()
	request := response.Request

	httpScope := getCurrent(ctx).(*httpCall)

	duration := clock.Get().Since(httpScope.StartedAt)

	statusCodeField := logging.HTTPStatusCode(response.StatusCode())
	durationField := logging.DurationMS(duration)

	statusCode := response.StatusCode()
	switch {
	case statusCode < 400:
		httpScope.logger.Info("success http response", statusCodeField, durationField)
	case statusCode < 500:
		httpScope.logger.Info("error http response", statusCodeField, durationField)
	default:
		httpScope.logger.Error("server error http response", statusCodeField, durationField)
	}

	metrics.HTTPCallFinished.WithLabelValues(
		metrics.HTTPCallFinishedLabels(request.Method, request.URL, response.StatusCode())...,
	).Inc()
	metrics.HTTPCallDuration.WithLabelValues(
		metrics.HTTPCallDurationLabels(request.Method, request.URL, response.StatusCode())...,
	).Observe(float64(duration.Milliseconds()))

	return nil
}

func FinishHTTPCallErr(request *resty.Request, err error) {
	httpScope := getCurrent(request.Context()).(*httpCall)
	if httpScope == nil {
		panic("Scope must be set")
	}
	httpScope.logger.Error("error http request", log.Error(err))

	metrics.HTTPCallError.WithLabelValues(
		metrics.HTTPCallErrorLabels(request.Method, request.URL, err)...,
	).Inc()
}
