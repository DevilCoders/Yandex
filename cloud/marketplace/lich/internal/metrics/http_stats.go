package metrics

import (
	"context"
	"fmt"
	"net/http"
	"strconv"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

type restHandlerMetrics struct {
	counter      metrics.Counter
	ratedCounter metrics.Counter

	timer metrics.Timer
}

type tags map[string]string

type handlerMetricsKey struct {
	handlerName string
	status      int
}

type handlersMetrics map[handlerMetricsKey]*restHandlerMetrics

// NOTE: 50 buckets maximum according to Solomon documentations.
var defaultResponsesBucket = metrics.NewDurationBuckets(
	time.Millisecond,
	5*time.Millisecond,
	10*time.Millisecond,
	20*time.Millisecond,
	30*time.Millisecond,
	40*time.Millisecond,
	50*time.Millisecond,
	75*time.Millisecond,
	100*time.Millisecond,
	125*time.Millisecond,
	200*time.Millisecond,
	250*time.Millisecond,
	500*time.Millisecond,
	750*time.Millisecond,
	1000*time.Millisecond,
	1250*time.Millisecond,
	1500*time.Millisecond,
	2000*time.Millisecond,
	2500*time.Millisecond,
	3000*time.Millisecond,
	4000*time.Millisecond,
	5000*time.Millisecond,
	6000*time.Millisecond,
	7000*time.Millisecond,
	8000*time.Millisecond,
	9000*time.Millisecond,
	10000*time.Millisecond,
	15000*time.Millisecond,
	20000*time.Millisecond,
	25000*time.Millisecond,
	30000*time.Millisecond,
	40000*time.Millisecond,
	50000*time.Millisecond,
	300000*time.Millisecond,
)

func (h *Hub) CommitHTTPRequest(ctx context.Context, handlerName string, httpStatus int, duration time.Duration) {
	scopedLogger := ctxtools.LoggerWith(ctx, log.String("handler_name", handlerName), log.Int("http_status", httpStatus))

	metrics := h.handlersMetrics[handlerMetricsKey{handlerName, httpStatus}]
	if metrics == nil {
		scopedLogger.Warn("unexpected rest handler status code")

		metrics = h.registerHTTPMetric(handlerName, httpStatus)

		h.mu.Lock()
		defer h.mu.Unlock()

		key := handlerMetricsKey{
			handlerName: handlerName,
			status:      httpStatus,
		}

		h.handlersMetrics[key] = metrics
	}

	metrics.counter.Inc()
	metrics.ratedCounter.Inc()
	metrics.timer.RecordDuration(duration)

	scopedLogger.Debug("handler metrics has been committed")
}

// RegisterHTTPHandler init handler's metrics, note the you shouldn't call this method concurrently.
func (h *Hub) RegisterHTTPHandler(handlerName string) {
	for httpStatus := 200; httpStatus <= 599; httpStatus++ {
		if description := http.StatusText(httpStatus); description != "" {
			key := handlerMetricsKey{
				handlerName: handlerName,
				status:      httpStatus,
			}

			h.handlersMetrics[key] = h.registerHTTPMetric(handlerName, httpStatus)
		}
	}
}

func (h *Hub) registerHTTPMetric(handlerName string, httpStatus int) *restHandlerMetrics {
	subregistry := h.registry.WithPrefix("http").WithTags(tags{
		"http_status":       strconv.FormatInt(int64(httpStatus), 10),
		"http_status_class": makeHTTPStatusCategory(httpStatus),
		"handler_name":      handlerName,
	})

	ratedCounter := subregistry.Counter("call_count_rated")
	solomon.Rated(ratedCounter)

	counter := subregistry.Counter("call_count")

	histogram := subregistry.DurationHistogram("handlers_timings", defaultResponsesBucket)
	solomon.Rated(histogram)

	return &restHandlerMetrics{
		counter:      counter,
		ratedCounter: ratedCounter,

		timer: histogram,
	}
}

func makeHTTPStatusCategory(status int) string {
	return fmt.Sprintf("%1dxx", status/100)
}
