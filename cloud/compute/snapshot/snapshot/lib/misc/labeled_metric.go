package misc

import (
	"context"
	"sync"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
)

var labels = []string{operationID, computeTaskID, taskID, format}

const (
	defaultOPTimeout   = 5 * time.Minute
	defaultGCSleepTime = time.Minute

	operationID   = "operation_id"
	computeTaskID = "compute_task_id"
	taskID        = "task_id"
	format        = "format"

	defaultOperationID   = "empty_id"
	defaultComputeTaskID = "empty_compute_task_id"
	defaultTaskID        = "empty_task_id"
	defaultFormat        = ""
)

type labeledMetric interface {
	GetObserver(labels metriclabels.Labels) prometheus.Observer
	DetachObserver(labels metriclabels.Labels)
}

type labeledHistogram struct {
	*prometheus.HistogramVec
}

func (l *labeledHistogram) GetObserver(labels metriclabels.Labels) prometheus.Observer {
	obs, err := l.GetMetricWith(ToLabels(labels))
	if err != nil {
		// accuired labels and histogramVec labels does not match
		panic(err)
	}
	return obs
}

func (l *labeledHistogram) DetachObserver(labels metriclabels.Labels) {
	l.Delete(ToLabels(labels))
}

type labeledGauge struct {
	*prometheus.GaugeVec
}

func (l *labeledGauge) GetObserver(labels metriclabels.Labels) prometheus.Observer {
	g, err := l.GetMetricWith(ToLabels(labels))
	if err != nil {
		panic(err)
	}
	return &labeledGaugeObserver{Gauge: g}
}

func (l *labeledGauge) DetachObserver(labels metriclabels.Labels) {
	l.Delete(ToLabels(labels))
}

type labeledGaugeObserver struct {
	prometheus.Gauge
}

func (l labeledGaugeObserver) Observe(val float64) {
	l.Set(val)
}

type labeledSummary struct {
	*prometheus.SummaryVec
}

func (l *labeledSummary) GetObserver(labels metriclabels.Labels) prometheus.Observer {
	obs, err := l.GetMetricWith(ToLabels(labels))
	if err != nil {
		panic(err)
	}
	return obs
}

func (l *labeledSummary) DetachObserver(labels metriclabels.Labels) {
	l.Delete(ToLabels(labels))
}

type compoundMetric struct {
	prometheus.Histogram
	labeled labeledMetric

	mtx            sync.Mutex
	lastOccurrence map[metriclabels.Labels]time.Time

	gcSleepTime time.Duration
	opTimeout   time.Duration

	clock clockwork.Clock
}

type compoundObserver struct {
	all   prometheus.Observer
	label prometheus.Observer
}

func SetDefault(labels metriclabels.Labels) metriclabels.Labels {
	if labels.OperationID == "" {
		labels.OperationID = defaultOperationID
	}
	if labels.ComputeTaskID == "" {
		labels.ComputeTaskID = defaultComputeTaskID
	}
	if labels.TaskID == "" {
		labels.TaskID = defaultTaskID
	}
	if labels.Format == "" {
		labels.Format = defaultFormat
	}
	return labels
}

func ToLabels(labels metriclabels.Labels) prometheus.Labels {
	return prometheus.Labels{
		operationID:   labels.OperationID,
		computeTaskID: labels.ComputeTaskID,
		taskID:        labels.TaskID,
		format:        labels.Format,
	}
}

func (o *compoundObserver) Observe(val float64) {
	if o.all != nil {
		o.all.Observe(val)
	}
	if o.label != nil {
		o.label.Observe(val)
	}

}

func (l *compoundMetric) Observer(ctx context.Context) prometheus.Observer {
	labels := metriclabels.Get(ctx)

	if !labels.DetailLabels {
		labels = metriclabels.Labels{Format: labels.Format}
	}

	labels = SetDefault(labels)

	l.mtx.Lock()
	defer l.mtx.Unlock()
	l.lastOccurrence[labels] = l.clock.Now()
	obs := l.labeled.GetObserver(labels)
	return &compoundObserver{all: l.Histogram, label: obs}
}

func (l *compoundMetric) ObserveContext(ctx context.Context, val float64) {
	l.Observer(ctx).Observe(val)
}

func (l *compoundMetric) clearOldLabels() {
	l.mtx.Lock()
	defer l.mtx.Unlock()

	lastOK := l.clock.Now().Add(-l.opTimeout)

	for op, lastTime := range l.lastOccurrence {
		if lastTime.Before(lastOK) {
			l.labeled.DetachObserver(op)
			delete(l.lastOccurrence, op)
		}
	}
}

func (l *compoundMetric) Start() {
	for range l.clock.NewTicker(l.gcSleepTime).Chan() {
		l.clearOldLabels()
	}
}
