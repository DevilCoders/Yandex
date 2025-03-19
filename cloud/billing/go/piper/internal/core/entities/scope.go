package entities

import (
	"time"
)

// ProcessingScope stores common data about one processing loop,
// i.e. source name, handling start time, etc.
type ProcessingScope struct {
	SourceName string // yc-smth/billing-system-topic-0
	SourceType string // logbroker-grpc
	SourceID   string // rt3.vla--yc-smth--billing-system-topic:99
	StartTime  time.Time

	Hostname string

	MetricsGrace time.Duration
	Pipeline     string

	MinMessageOffset uint64
	MaxMessageOffset uint64
}
