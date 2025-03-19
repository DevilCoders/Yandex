package entities

import "time"

type IncorrectMetricDump struct {
	SequenceID uint
	MetricID   string

	Reason        string
	ReasonComment string

	MetricSourceID string
	SourceID       string
	SourceName     string
	Hostname       string
	UploadedAt     time.Time

	MetricSchema     string
	MetricResourceID string
	MetricData       string
	RawMetric        string
}
