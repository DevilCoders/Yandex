package metrics

import "github.com/prometheus/client_golang/prometheus"

func init() {
	register(IncomingMetrics)
	register(InvalidMetrics)
	register(ProcessedMetrics)
	register(ProducerWriteLag)
	register(ProducerProcessLag)
	register(CumulativeChanges)
}

var (
	IncomingMetrics = prometheus.NewGaugeVec(prometheus.GaugeOpts{Name: "incoming_metrics"}, []string{"svc", "source", "partition"})
	InvalidMetrics  = prometheus.NewGaugeVec(
		prometheus.GaugeOpts{Name: "invalid_metrics"}, []string{"svc", "source", "partition", "reason"},
	)
	ProcessedMetrics = prometheus.NewGaugeVec(prometheus.GaugeOpts{Name: "processed_metrics"}, []string{"svc", "source", "partition"})
)

func IncomingMetricsLabels(service, source, partition string) []string {
	return []string{service, source, partition}
}

func InvalidMetricsLabels(service, source, partition, reason string) []string {
	return []string{service, source, partition, reason}
}

func ProcessedMetricsLabels(service, source, partition string) []string {
	return []string{service, source, partition}
}

var (
	ProducerWriteLag = prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "producer_write_lag",
			Buckets: timeBucketsLag,
		}, []string{"svc", "source", "schema"},
	)
	ProducerProcessLag = prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "processing_lag",
			Buckets: timeBucketsLag,
		}, []string{"svc", "source", "schema"},
	)
)

func SchemaLagMetrics(service, source, schema string) []string {
	return []string{service, source, schema}
}

var CumulativeChanges = prometheus.NewGaugeVec(
	prometheus.GaugeOpts{Name: "cumulative_changes"}, []string{"svc", "source"},
)

func CumulativeChangesLabels(service, source string) []string {
	return []string{service, source}
}
