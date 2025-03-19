package metrics

import (
	"strconv"

	"github.com/prometheus/client_golang/prometheus"
)

func init() {
	register(HTTPCallDuration)
	register(HTTPCallStarted)
	register(HTTPCallFinished)
	register(HTTPCallError)
}

var (
	HTTPCallDuration = prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "http_client_duration_ms",
			Buckets: rpcMillisecondBuckets,
		}, []string{"method", "url", "code"},
	)

	HTTPCallStarted = prometheus.NewCounterVec(
		prometheus.CounterOpts{Name: "http_client_started"}, []string{"method", "url_pattern"},
	)

	HTTPCallFinished = prometheus.NewCounterVec(
		prometheus.CounterOpts{Name: "http_client_finished_ok"}, []string{"method", "url_pattern", "code"},
	)

	HTTPCallError = prometheus.NewCounterVec(
		prometheus.CounterOpts{Name: "http_client_finished_error"}, []string{"method", "url_pattern", "error"},
	)
)

func HTTPCallStartedLabels(method, url string) []string {
	return []string{method, url}
}

func HTTPCallFinishedLabels(method, url string, code int) []string {
	return []string{method, url, strconv.Itoa(code)}
}

func HTTPCallDurationLabels(method, url string, code int) []string {
	return []string{method, url, strconv.Itoa(code)}
}

func HTTPCallErrorLabels(method, url string, err error) []string {
	return []string{method, url, err.Error()}
}
