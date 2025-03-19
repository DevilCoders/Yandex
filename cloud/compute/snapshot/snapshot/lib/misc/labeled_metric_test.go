package misc

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/prometheus/client_golang/prometheus"
	dto "github.com/prometheus/client_model/go"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
)

func testHist(name string) (*compoundMetric, clockwork.FakeClock) {
	sample := BoundedBuckets(6, 6)
	h := prometheus.NewHistogram(prometheus.HistogramOpts{
		Name:    name,
		Buckets: sample,
	})

	vecName := fmt.Sprintf("%s_labels", name)
	hIDs := prometheus.NewHistogramVec(prometheus.HistogramOpts{
		Name:    vecName,
		Buckets: sample,
	}, labels)

	clock := clockwork.NewFakeClock()
	hist := &compoundMetric{
		Histogram:      h,
		labeled:        &labeledHistogram{HistogramVec: hIDs},
		lastOccurrence: map[metriclabels.Labels]time.Time{},
		clock:          clock,
	}
	return hist, clock

}

func TestLabeledHistogram(t *testing.T) {
	a := assert.New(t)
	hist, _ := testHist("unit_test")

	hist.Observe(0.5)
	ctx := metriclabels.WithOperationID(context.Background(), "opid")
	ctx = metriclabels.WithUseLabels(ctx, true)
	hist.ObserveContext(ctx, 0.5)

	var d dto.Metric
	err := hist.Histogram.Write(&d)
	a.NoError(err)
	a.Equal(uint64(2), *d.Histogram.SampleCount, "two observation in general histogram")

	collected := make(chan prometheus.Metric, 10)
	l := hist.labeled.(*labeledHistogram)
	l.HistogramVec.Collect(collected)

	close(collected)

	a.Equal(1, len(collected), "one observation in labeled histogram")

	for m := range collected {
		var d dto.Metric
		err := m.Write(&d)
		a.NoError(err)
		a.Equal(uint64(1), *d.Histogram.SampleCount)
		a.Equal(len(labels), len(d.Label))
		for _, l := range d.Label {
			if *l.Name == operationID {
				a.Equal("opid", *l.Value, "operation id label must be same")
			}
		}
	}
}

func TestRemoveOldOperations(t *testing.T) {
	a := assert.New(t)
	hist, c := testHist("test")
	hist.opTimeout = 5 * time.Millisecond

	ctx := metriclabels.WithOperationID(context.Background(), "id")
	ctx = metriclabels.WithUseLabels(ctx, true)
	obs := hist.Observer(ctx)

	// on first observation labeled observer is remembered
	obs.Observe(1)

	collected := make(chan prometheus.Metric, 10)
	l := hist.labeled.(*labeledHistogram)
	l.HistogramVec.Collect(collected)

	close(collected)

	a.Equal(1, len(collected), "one has been observed")

	for m := range collected {
		var d dto.Metric
		err := m.Write(&d)
		a.NoError(err)
		a.Equal(uint64(1), *d.Histogram.SampleCount)
		a.Equal(len(labels), len(d.Label))
		for _, l := range d.Label {
			if *l.Name == operationID {
				a.Equal("id", *l.Value)
			}
		}
	}

	c.Advance(2 * time.Millisecond)
	hist.clearOldLabels()

	obs.Observe(2)

	collected = make(chan prometheus.Metric, 10)
	l.HistogramVec.Collect(collected)

	close(collected)

	a.Equal(1, len(collected), "not enough time passed, still one observed")

	for m := range collected {
		var d dto.Metric
		err := m.Write(&d)
		a.NoError(err)
		a.Equal(uint64(2), *d.Histogram.SampleCount)
		a.Equal(len(labels), len(d.Label))
		for _, l := range d.Label {
			if *l.Name == "id" {
				a.Equal("id", *l.Value)
			}
		}
	}

	c.Advance(10 * time.Millisecond)
	hist.clearOldLabels()

	obs.Observe(3)

	collected = make(chan prometheus.Metric, 10)
	l.HistogramVec.Collect(collected)

	close(collected)

	a.Equal(0, len(collected), "observer is old and has to be detached")
}

func TestLabeledHistogramStress(t *testing.T) {
	a := assert.New(t)
	hist, c := testHist("stress_test")

	tasksLen := 100
	operationsLen := 10
	workers := 20

	operations := make([]string, operationsLen)
	for i := 0; i < operationsLen; i++ {
		operations[i] = fmt.Sprintf("operation_%d", i)
	}

	tasks := make(chan string, tasksLen)
	results := make(chan struct{}, tasksLen)

	for i := 0; i < workers; i++ {
		go func() {
			for label := range tasks {
				c := metriclabels.WithOperationID(context.Background(), label)
				c = metriclabels.WithUseLabels(c, true)
				hist.ObserveContext(c, 1)
				results <- struct{}{}
			}
		}()
	}

	for i, k := 0, 0; i < tasksLen; i, k = i+1, (k+1)%operationsLen {
		tasks <- operations[k]
	}
	close(tasks)

	for i := 0; i < tasksLen; i++ {
		<-results
	}

	gathered := make(chan prometheus.Metric, operationsLen+1)
	l := hist.labeled.(*labeledHistogram)
	l.HistogramVec.Collect(gathered)
	close(gathered)
	a.Equal(operationsLen, len(gathered), "check all metric collected, each op id get required data")

	allOP := map[string]struct{}{}
	for _, op := range operations {
		allOP[op] = struct{}{}
	}
	perOP := uint64(tasksLen / operationsLen)
	var d dto.Metric

	for m := range gathered {
		var d dto.Metric
		err := m.Write(&d)
		a.NoError(err)
		a.Equal(perOP, *d.Histogram.SampleCount)
		a.Equal(len(labels), len(d.Label))
		for _, l := range d.Label {
			if *l.Name == operationID {
				delete(allOP, *l.Value)
			}
		}

	}

	a.Equal(map[string]struct{}{}, allOP, "check all operations found")

	c.Advance(100 * time.Millisecond)
	hist.clearOldLabels()

	gathered = make(chan prometheus.Metric, operationsLen+1)
	l.HistogramVec.Collect(gathered)
	close(gathered)
	a.Equal(0, len(gathered))

	d = dto.Metric{}
	err := hist.Write(&d)
	a.NoError(err)
	a.Equal(uint64(tasksLen), *d.Histogram.SampleCount, "all data was collected in regular histogram too")
}

func TestMultivaluedLabels(t *testing.T) {
	a := assert.New(t)
	hist, c := testHist("test_multivalued")
	hist.opTimeout = 10 * time.Millisecond
	cases := []metriclabels.Labels{
		{OperationID: "", ComputeTaskID: "", TaskID: "", DetailLabels: true},
		{OperationID: "id", ComputeTaskID: "", TaskID: "", DetailLabels: true},
		{OperationID: "", ComputeTaskID: "id", TaskID: "", DetailLabels: true},
		{OperationID: "", ComputeTaskID: "", TaskID: "id", DetailLabels: true},
		{OperationID: "id", ComputeTaskID: "id", TaskID: "", DetailLabels: true},
		{OperationID: "id", ComputeTaskID: "", TaskID: "id", DetailLabels: true},
		{OperationID: "", ComputeTaskID: "id", TaskID: "id", DetailLabels: true},
		{OperationID: "id", ComputeTaskID: "id", TaskID: "id", DetailLabels: true},
	}
	hasInRes := map[metriclabels.Labels]struct{}{}

	for _, c := range cases {
		hist.ObserveContext(metriclabels.WithMetricData(context.Background(), c), 1)
		if c.OperationID == "" {
			c.OperationID = defaultOperationID
		}
		if c.ComputeTaskID == "" {
			c.ComputeTaskID = defaultComputeTaskID
		}
		if c.TaskID == "" {
			c.TaskID = defaultTaskID
		}
		// useLabel is not actual label so it is must to be explicitly ignored
		c.DetailLabels = false
		hasInRes[c] = struct{}{}
	}

	res := make(chan prometheus.Metric, 9)
	l := hist.labeled.(*labeledHistogram)
	l.HistogramVec.Collect(res)
	close(res)
	a.Equal(len(cases), len(res))

	for m := range res {
		var d dto.Metric
		err := m.Write(&d)
		a.NoError(err)
		// we cannot ensure order of labels
		var res metriclabels.Labels
		for _, l := range d.Label {
			switch *l.Name {
			case operationID:
				res.OperationID = *l.Value
			case computeTaskID:
				res.ComputeTaskID = *l.Value
			case taskID:
				res.TaskID = *l.Value
			}
		}
		delete(hasInRes, res)
	}

	a.Equal(0, len(hasInRes), "check all cases were found")

	c.Advance(5 * time.Millisecond)
	hist.ObserveContext(metriclabels.WithMetricData(context.Background(),
		metriclabels.Labels{OperationID: "id", ComputeTaskID: "id", TaskID: "id", DetailLabels: true}), 2)
	c.Advance(6 * time.Millisecond)
	hist.clearOldLabels()

	res = make(chan prometheus.Metric, 2)
	l.HistogramVec.Collect(res)
	close(res)
	a.Equal(1, len(res), "labels are to be independent from each other")
	for m := range res {
		var d dto.Metric
		err := m.Write(&d)
		a.NoError(err)
		for _, l := range d.Label {
			switch *l.Name {
			case operationID:
				a.Equal("id", *l.Value)
			case computeTaskID:
				a.Equal("id", *l.Value)
			case taskID:
				a.Equal("id", *l.Value)
			}
		}
	}
}
