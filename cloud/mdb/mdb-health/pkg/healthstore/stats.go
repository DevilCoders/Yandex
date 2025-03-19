package healthstore

import "github.com/prometheus/client_golang/prometheus"

const (
	prometheusNamespace = "health"
	prometheusSubsystem = "healthstore"
)

type stat struct {
	backendErrors          prometheus.Counter
	storeErrors            prometheus.Counter
	iterationTime          prometheus.Histogram
	iterationTimeOverLimit prometheus.Histogram
	batchSize              prometheus.Gauge
}

var stats = newStats()

func newStats() *stat {
	s := &stat{
		backendErrors:          newCounter("backend_errors"),
		storeErrors:            newCounter("store_errors"),
		iterationTime:          newHistogram("iteration_sec"),
		iterationTimeOverLimit: newHistogram("iteration_sec_over_limit"),
		batchSize:              newGauge("batch_size"),
	}
	prometheus.MustRegister(
		s.backendErrors,
		s.storeErrors,
		s.iterationTime,
		s.iterationTimeOverLimit,
		s.batchSize,
	)

	return s
}

func newCounter(name string) prometheus.Counter {
	return prometheus.NewCounter(prometheus.CounterOpts{
		Namespace: prometheusNamespace,
		Subsystem: prometheusSubsystem,
		Name:      name,
	})
}

func newGauge(name string) prometheus.Gauge {
	return prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: prometheusNamespace,
		Subsystem: prometheusSubsystem,
		Name:      name,
	})
}

func newHistogram(name string) prometheus.Histogram {
	return prometheus.NewHistogram(prometheus.HistogramOpts{
		Namespace: prometheusNamespace,
		Subsystem: prometheusSubsystem,
		Name:      name,
	})
}
