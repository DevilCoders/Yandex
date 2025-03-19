package server

import (
	"net/http/pprof"

	// ensure that metrics are created
	"net/http"

	"github.com/prometheus/client_golang/prometheus/promhttp"

	_ "a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

var metricsServer http.Server
var metricsServerMux *http.ServeMux

func init() {
	metricsServerMux = http.NewServeMux()
	metricsServerMux.Handle("/metrics", promhttp.Handler())

	// go profiler
	metricsServerMux.HandleFunc("/debug/pprof/", pprof.Index)
	metricsServerMux.HandleFunc("/debug/pprof/cmdline", pprof.Cmdline)
	metricsServerMux.HandleFunc("/debug/pprof/profile", pprof.Profile)
	metricsServerMux.HandleFunc("/debug/pprof/symbol", pprof.Symbol)
	metricsServerMux.HandleFunc("/debug/pprof/trace", pprof.Trace)

	metricsServer.Handler = metricsServerMux
}
