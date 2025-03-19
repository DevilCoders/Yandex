package misc

import (
	"sync"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
)

var metricAvgSpeedSizeOfBuffer = 100

// NewMetricAvgSpeed creates new MetricAvgSpeed and starts goroutine with metrics procession
func NewMetricAvgSpeed(param MetricAvgSpeedOpts) *MetricAvgSpeed {
	mas := &MetricAvgSpeed{
		toProcess:    make(chan MetricAvgSpeedBatch, metricAvgSpeedSizeOfBuffer),
		allDurations: 0,
		allDataBytes: 0,
		gauge: promauto.NewGauge(prometheus.GaugeOpts{
			Name: param.Name,
			Help: param.Help,
		}),
	}
	go mas.processObservations()

	return mas
}

// MetricAvgSpeed metric which processes amount of information and time spent to process it, copmutes speed and send to prometheus
type MetricAvgSpeed struct {
	mux          sync.Mutex
	toProcess    chan MetricAvgSpeedBatch
	allDurations time.Duration
	allDataBytes int
	gauge        prometheus.Gauge
}

// MetricAvgSpeedOpts options for prometheus metric
type MetricAvgSpeedOpts struct {
	Name string
	Help string
}

// MetricAvgSpeedBatch is struct which describes one data transfer
type MetricAvgSpeedBatch struct {
	dataAmount int
	spentTime  time.Duration
}

// Observe saves new number to metric
func (mas *MetricAvgSpeed) Observe(dataAmount int, spentTime time.Duration) {
	batch := MetricAvgSpeedBatch{
		dataAmount: dataAmount,
		spentTime:  spentTime,
	}
	// lose value if channel is busy
	select {
	case mas.toProcess <- batch:
	default:
	}
}

// processObservations precesses batches of metrics and saves it to internal storage
func (mas *MetricAvgSpeed) processObservations() {
	for batch := range mas.toProcess {
		mas.mux.Lock()

		mas.allDurations += batch.spentTime
		mas.allDataBytes += batch.dataAmount

		mas.mux.Unlock()
	}
}

// DumpMetric starts getting final avg value
func (mas *MetricAvgSpeed) DumpMetric() {
	mas.mux.Lock()
	defer mas.mux.Unlock()

	var metricValueBytesPerSec float64

	if mas.allDurations.Seconds() != 0 {
		metricValueBytesPerSec = float64(mas.allDataBytes) / mas.allDurations.Seconds()
	}
	mas.gauge.Set(metricValueBytesPerSec)
	mas.allDataBytes = 0
	mas.allDurations = 0
}
