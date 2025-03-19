package misc

import (
	"fmt"
	"testing"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	dto "github.com/prometheus/client_model/go"
	"github.com/stretchr/testify/assert"
)

func TestMetrics(t *testing.T) {
	a := assert.New(t)
	data, err := prometheus.DefaultGatherer.Gather()
	a.NoError(err)

	totalInfo := 0
	foundNames := metricsNames{}
	for _, d := range data {
		foundNames.add(*d.Name)
		for _, m := range d.Metric {
			// all non histogram are single value
			thisMetricCount := 1
			if m.Histogram != nil {
				thisMetricCount = len(m.Histogram.Bucket)
			}
			if m.Summary != nil {
				thisMetricCount = len(m.Summary.Quantile)
			}

			// 200 labels per labeled allowed
			if len(m.Label) != 0 {
				thisMetricCount *= 200
			}

			totalInfo += thisMetricCount
		}
	}
	a.Greater(10000, totalInfo, "default per host metric quota")
	a.Less(0, len(foundNames), "we need to have at least one metric")
	fmt.Println(foundNames)

	for n := range names {
		_, ok := foundNames[n]
		a.True(ok, fmt.Sprintf("metric %s not found", n))
	}
}

func TestTimer(t *testing.T) {
	a := assert.New(t)
	buckets := BoundedBuckets(0.06, 6)
	a.InDeltaSlice([]float64{0.01, 0.02, 0.03, 0.04, 0.05, 0.06}, buckets, 0.00000001,
		"check buckets are created correctly")
	timer := MustRegisterTimer("test_timer", buckets)
	timer.Observe(time.Millisecond * 15)
	timer.ObserveSince(time.Now().Add(-time.Millisecond * 15))
	info := dto.Metric{}
	err := timer.hist.Write(&info)
	a.NoError(err)
	// 15 milliseconds > 0.01 second but less then 0.02, 0.03, 0.04, 0.05, 0.06
	a.Equal(uint64(2), *info.Histogram.SampleCount)
	a.Equal(uint64(0), *info.Histogram.Bucket[0].CumulativeCount)
	for i := 1; i < len(info.Histogram.Bucket); i++ {
		a.Equal(uint64(2), *info.Histogram.Bucket[i].CumulativeCount)
	}
}

func TestBoundedStepBuckets(t *testing.T) {
	a := assert.New(t)
	buckets := BoundedStepBuckets(1, 10, 1)
	a.Equal([]float64{1, 2, 3, 4, 5, 6, 7, 8, 9}, buckets)

}
