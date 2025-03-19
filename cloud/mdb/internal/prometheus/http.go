package prometheus

import (
	"net/http"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"

	"a.yandex-team.ru/library/go/core/log"
)

// Middleware adds instrumentation to http handlers
func Middleware(next http.Handler, logger log.Logger) http.Handler {
	inFlightGauge := prometheus.NewGauge(prometheus.GaugeOpts{
		Name: "in_flight_requests",
		Help: "A gauge of requests currently being served by the wrapped handler.",
	})

	counter := prometheus.NewCounterVec(
		prometheus.CounterOpts{
			Name: "api_requests_total",
			Help: "A counter for requests to the wrapped handler.",
		},
		[]string{"code", "method"},
	)

	duration := prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "request_duration_seconds",
			Help:    "A histogram of latencies for requests.",
			Buckets: []float64{.001, .002, .005, .01, .02, .05, .1, .2, .5, 1, 2, 5, 10},
		},
		[]string{"code", "method"},
	)

	requestSize := prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "request_size_bytes",
			Help:    "A histogram of request sizes for requests.",
			Buckets: []float64{0, 50, 100, 200, 500, 1000, 2000, 5000, 10000},
		},
		[]string{"code", "method"},
	)

	responseSize := prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "response_size_bytes",
			Help:    "A histogram of response sizes for requests.",
			Buckets: []float64{0, 50, 100, 250, 500, 1000, 2500, 5000, 10000},
		},
		[]string{"code", "method"},
	)

	prometheus.MustRegister(inFlightGauge, counter, duration, responseSize)

	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		handler := promhttp.InstrumentHandlerInFlight(inFlightGauge,
			promhttp.InstrumentHandlerDuration(duration,
				promhttp.InstrumentHandlerCounter(counter,
					promhttp.InstrumentHandlerRequestSize(requestSize,
						promhttp.InstrumentHandlerResponseSize(responseSize, next),
					),
				),
			),
		)

		handler.ServeHTTP(w, r)
	})
}
