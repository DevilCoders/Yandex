package entities

import (
	"time"

	"github.com/gofrs/uuid"
)

type InvalidMetric struct {
	Metric

	SourceWT time.Time

	IncorrectRawMessage
}

type IncorrectRawMessage struct {
	Reason        MetricFailReason
	ReasonComment string

	RawMetric  []byte
	UploadTime time.Time

	MessageWriteTime time.Time
	MessageOffset    uint64
}

func (m IncorrectRawMessage) UploadedAt() time.Time {
	if m.UploadTime.IsZero() {
		return time.Now()
	}
	return m.UploadTime
}

func (m InvalidMetric) Offset() uint64 { return m.MessageOffset }

type InvalidMetricID struct {
	ID         string
	Schema     string
	SourceID   string
	ResourceID string
}

func (m InvalidMetric) MetricID() InvalidMetricID {
	result := InvalidMetricID{
		ID: uuid.Must(uuid.NewV4()).String(),
	}
	if m.Metric == nil {
		return result
	}
	switch mtr := m.Metric.(type) {
	case SourceMetric:
		result.ID = mtr.MetricID
		result.Schema = mtr.Schema
		result.SourceID = mtr.SourceID
		result.ResourceID = mtr.ResourceID
	case EnrichedMetric:
		result.ID = mtr.MetricID
		result.Schema = mtr.Schema
		result.SourceID = mtr.SourceID
		result.ResourceID = mtr.ResourceID
	}
	return result
}
