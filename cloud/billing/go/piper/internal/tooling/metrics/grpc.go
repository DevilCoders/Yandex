package metrics

import (
	"strconv"

	"github.com/prometheus/client_golang/prometheus"
	"google.golang.org/grpc/codes"
)

func init() {
	register(GRPCDuration)
	register(GRPCStarted)
}

var (
	GRPCDuration = prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "grpc_duration",
			Buckets: timeBuckets,
		}, []string{"method", "code"},
	)
	GRPCStarted = prometheus.NewGaugeVec(
		prometheus.GaugeOpts{Name: "grpc_started"}, []string{"method"},
	)
)

func GRPCStartLabels(method string) []string {
	return []string{method}
}

func GRPCDoneLabels(method string, code codes.Code) []string {
	return []string{method, strconv.Itoa(int(code))}
}
