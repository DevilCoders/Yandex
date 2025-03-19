package metrics

import (
	"github.com/prometheus/client_golang/prometheus"
)

func init() {
	register(QueryDuration)
	register(QueryStarted)
}

var (
	QueryDuration = prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "db_queries_duration",
			Buckets: timeBuckets,
		}, []string{"query_name", "success"},
	)
	QueryStarted = prometheus.NewGaugeVec(
		prometheus.GaugeOpts{Name: "db_queries_started"}, []string{"query_name"},
	)
)

func QueryStartLabels(queryName string) []string {
	return []string{queryName}
}

func QueryDoneLabels(queryName string, success bool) []string {
	successStr := "no"
	if success {
		successStr = "yes"
	}
	return []string{queryName, successStr}
}
