package metrics

import (
	"strconv"

	"github.com/prometheus/client_golang/prometheus"
	"google.golang.org/grpc/codes"
)

func init() {
	register(GRPCRequestDuration)
	register(GRPCRequestStarted)
	register(GRPCRequestFinished)
}

var (
	GRPCRequestDuration = prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "grpc_server_duration_ms",
			Buckets: rpcMillisecondBuckets,
		}, []string{"method", "code"},
	)

	GRPCRequestStarted = prometheus.NewCounterVec(
		prometheus.CounterOpts{Name: "grpc_server_started"}, []string{"method"},
	)

	GRPCRequestFinished = prometheus.NewCounterVec(
		prometheus.CounterOpts{Name: "grpc_server_finished"}, []string{"method", "code"},
	)
)

func GRPCRequestStartedLabels(method string) []string {
	return []string{method}
}

func GRPCRequestFinishedLabels(method string, code codes.Code) []string {
	return []string{method, strconv.Itoa(int(code))}
}

func GRPCRequestDurationLabels(method string, code codes.Code) []string {
	return []string{method, strconv.Itoa(int(code))}
}
