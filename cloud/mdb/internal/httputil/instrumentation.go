package httputil

import (
	"net/http"
	_ "net/http/pprof" // Populate http.DefaultServeMux

	"github.com/prometheus/client_golang/prometheus/promhttp"

	"a.yandex-team.ru/cloud/mdb/internal/prometheus"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	// PrometheusEndpoint is where prometheus metrics can be found
	PrometheusEndpoint = "/metrics"
	// YasmEndpoint is where yasm stats can be found
	YasmEndpoint = "/stats"
)

func DefaultInstrumentationConfig() ServeConfig {
	cfg := DefaultServeConfig()
	cfg.Addr = "[::1]:6060"
	return cfg
}

// InstrumentationServer creates http server with pprof, prometheus and yasm endpoints
func InstrumentationServer(addr string, l log.Logger) *http.Server {
	mux := http.NewServeMux()
	// Redirect pprof prefix to default mux
	mux.Handle("/debug/pprof/", http.DefaultServeMux)
	mux.Handle(PrometheusEndpoint, promhttp.Handler())
	mux.Handle(YasmEndpoint, prometheus.YasmHandler(l))

	return &http.Server{Addr: addr, Handler: mux}
}
