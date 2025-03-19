package metrics

import "github.com/prometheus/client_golang/prometheus"

func init() {
	register(AppAlive)
}

var (
	AppAlive = prometheus.NewCounterVec(prometheus.CounterOpts{Name: "app_alive"}, []string{})
)
