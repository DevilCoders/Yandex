package metrics

import (
	"fmt"
	"net/http"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/collectors"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

func GetHandler() http.Handler {
	return promhttp.HandlerFor(reg, promhttp.HandlerOpts{})
}

func SetConfigVersion(version string) {
	ConfigVersion.Reset()
	ConfigVersion.WithLabelValues(version).Set(1)
}

func SetServiceVersion(version string) {
	ServiceVersion.Reset()
	ServiceVersion.WithLabelValues(version).Set(1)
}

func ResetMetrics() {
	for _, r := range resetables {
		r.Reset()
	}
}

var reg = prometheus.NewRegistry()

func init() {
	reg.MustRegister(
		collectors.NewGoCollector(),
		collectors.NewProcessCollector(collectors.ProcessCollectorOpts{Namespace: "sys"}),
		ConfigVersion,
		ServiceVersion,
	)
}

var (
	ConfigVersion  = prometheus.NewGaugeVec(prometheus.GaugeOpts{Name: "config_version"}, []string{"version"})
	ServiceVersion = prometheus.NewGaugeVec(prometheus.GaugeOpts{Name: "service_version"}, []string{"version"})
)

var resetables []reseter

type reseter interface {
	Reset()
}

type resetableCollector interface {
	prometheus.Collector
	reseter
}

func register(m resetableCollector) {
	reg.MustRegister(m)
	resetables = append(resetables, m)
}

var (
	timeBuckets = joinBuckets(
		durationBuckets(time.Millisecond, 10, 25, 50, 75, 100, 250, 500, 750),
		durationBuckets(time.Second, 1, 2.5, 5, 7.5, 10, 15, 20, 25, 30, 45),
		durationBuckets(time.Minute, 1, 2.5, 5, 7.5, 10),
	)
	timeBucketsLong = joinBuckets(
		timeBuckets,
		durationBuckets(time.Minute, 15, 20, 25, 30, 45, 60),
	)
	timeBucketsLag = joinBuckets(
		durationBuckets(time.Minute, 1, 5, 10, 15, 20, 25, 30, 45),
		durationBuckets(time.Hour, 1, 5, 10, 15, 20),
		durationBuckets(time.Hour*24, 1, 5, 10, 15, 20),
	)
)

func joinBuckets(bkts ...[]float64) (result []float64) {
	for _, b := range bkts {
		result = append(result, b...)
	}
	for i := range result {
		if i == 0 {
			continue
		}
		if result[i] <= result[i-1] {
			panic(fmt.Sprintf("buckets has incorrect order at index %d (%f <= %f)", i, result[i], result[i-1]))
		}
	}
	return result
}

func durationBuckets(d time.Duration, values ...float64) []float64 {
	microseconds := d.Microseconds()
	result := make([]float64, len(values))
	for i, v := range values {
		result[i] = v * float64(microseconds)
	}
	return result
}
