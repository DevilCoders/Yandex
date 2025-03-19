package metrics

import (
	"net/http"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/collectors"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

func init() {
	collectorsRegistry.MustRegister(
		collectors.NewGoCollector(),
		collectors.NewProcessCollector(collectors.ProcessCollectorOpts{Namespace: "sys"}),
	)
}

var (
	collectorsRegistry  = prometheus.NewRegistry()
	resetableCollectors []reseter
)

type reseter interface {
	Reset()
}

type resetableCollector interface {
	prometheus.Collector
	reseter
}

func register(m resetableCollector) {
	collectorsRegistry.MustRegister(m)
	resetableCollectors = append(resetableCollectors, m)
}

func GetHandler() http.Handler {
	return promhttp.HandlerFor(collectorsRegistry, promhttp.HandlerOpts{})
}

func ResetMetrics() {
	for _, r := range resetableCollectors {
		r.Reset()
	}
}
