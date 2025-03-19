package entities

import (
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

const (
	DefaultMetricVersion = "v1alpha1"
)

// Metric is binding interface for all kinds of metrics.
type Metric interface {
	isMetric()
	Offset() uint64
}

type SourceMetric struct {
	MetricID string
	Schema   string

	Version string

	CloudID     string
	FolderID    string
	AbcID       int64
	AbcFolderID string

	ResourceID       string
	BillingAccountID string

	SkuID string

	Labels Labels
	Tags   types.JSONAnything

	Usage MetricUsage

	SourceID string
	SourceWT time.Time

	MessageWriteTime time.Time
	MessageOffset    uint64
}

func (SourceMetric) isMetric()        {}
func (m SourceMetric) Offset() uint64 { return m.MessageOffset }

func (m SourceMetric) Clone() SourceMetric {
	origLabels := m.Labels
	m.Labels = Labels{
		User: make(map[string]string),
		// System: make(map[string]string),
	}
	for k, v := range origLabels.User {
		m.Labels.User[k] = v
	}
	// for k, v := range origLabels.System {
	// 	m.Labels.System[k] = v
	// }
	return m
}

type EnrichedMetric struct {
	SourceMetric
	SkuInfo

	Period              MetricPeriod
	PricingQuantity     decimal.Decimal128
	Products            []string
	ResourceBindingType ResourceBindingType
	TagsOverride        map[string]string

	MasterAccountID string
	ReshardingKey   string
}

func (m EnrichedMetric) Clone() EnrichedMetric {
	m.SourceMetric = m.SourceMetric.Clone()

	if m.TagsOverride != nil {
		tags := m.TagsOverride
		m.TagsOverride = make(map[string]string, len(tags))
		for k, v := range tags {
			m.TagsOverride[k] = v
		}
	}

	return m
}

type Labels struct {
	User map[string]string
	// System map[string]string
}

type HashedLabel struct {
	LabelsHash uint64
	LabelsData string
}

type MetricUsage struct {
	Quantity decimal.Decimal128
	Start    time.Time
	Finish   time.Time
	Unit     string
	Type     UsageType
	RawType  string
}

func (u MetricUsage) UsageTime() time.Time {
	if u.Start == u.Finish || u.Finish.IsZero() {
		return u.Finish
	}
	return u.Finish.Add(-time.Second)
}

type MetricPeriod struct {
	Start  time.Time
	Finish time.Time
}

type MetricsSchema struct {
	Schema       string
	RequiredTags []string
}

func (ms MetricsSchema) Empty() bool {
	return ms.Schema == ""
}

type MetricIdentity struct {
	Schema   string
	MetricID string
	Offset   uint64
}

func (m MetricIdentity) Cmp(o MetricIdentity) int {
	switch {
	case m == o:
		return 0
	case m.Schema < o.Schema:
		return -1
	case m.MetricID < o.MetricID:
		return -1
	case m.Offset < o.Offset:
		return -1
	}
	return 1
}

type PresenterMetric struct {
	EnrichedMetric

	LabelsHash            uint64
	Cost                  decimal.Decimal128
	Credit                decimal.Decimal128
	CudCredit             decimal.Decimal128
	MonetaryGrantCredit   decimal.Decimal128
	VolumeIncentiveCredit decimal.Decimal128
}
