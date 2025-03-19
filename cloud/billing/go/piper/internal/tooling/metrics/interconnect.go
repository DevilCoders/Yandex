package metrics

import (
	"github.com/prometheus/client_golang/prometheus"
)

func init() {
	register(InterconnectDuration)
	register(InterconnectStarted)
}

var (
	InterconnectDuration = prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "ic_request_duration",
			Buckets: timeBuckets,
		}, []string{"dst_system", "request_name", "success"},
	)
	InterconnectStarted = prometheus.NewGaugeVec(
		prometheus.GaugeOpts{Name: "ic_request_started"}, []string{"dst_system", "request_name"},
	)
)

func InterconnectStartLabels(dstSystem, requestName string) []string {
	return []string{dstSystem, requestName}
}

func InterconnectDoneLabels(dstSystem, requestName string, success bool) []string {
	successStr := "no"
	if success {
		successStr = "yes"
	}
	return []string{dstSystem, requestName, successStr}
}
