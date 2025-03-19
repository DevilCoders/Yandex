package juggler

import (
	"context"
	"fmt"
	"time"
)

//go:generate ../../scripts/mockgen.sh API

type RawEvent struct {
	ReceivedTime time.Time
	Status       string
	Service      string
	Host         string
	Description  string
}

type CheckState struct {
	AggregationTime time.Time
	ChangeTime      time.Time
	Description     string
	Host            string
	Service         string
	// Enum: [ACTUAL INVALID FLAPPING REWRITTEN_BY_AGGREGATOR CHILD_GROUP_INVALID AGGREGATOR_SKIP DOWNTIME_SKIP
	// DOWNTIME_FORCE_OK UNREACH_SKIP UNREACH_FORCE_OK NO_DATA_SKIP NO_DATA_FORCE_OK NO_DATA_FORCE_CRIT
	// NO_DATA_FORCE_WARN RECENT_EVENTS_FORCE_CRIT]
	// This enum is not enforced by this client
	StateKind string
	// Enum: [OK WARN CRIT INFO]
	// This enum is not enforced by this client
	Status string
}

func (ev RawEvent) String() string {
	return fmt.Sprintf("%s on %s:%s at %s: %s", ev.Status, ev.Host, ev.Service, ev.ReceivedTime.Format(time.RFC3339), ev.Description)
}

type API interface {
	// RawEvents return RawEvents for given host and service
	RawEvents(ctx context.Context, hosts, services []string) ([]RawEvent, error)
	GetChecksState(ctx context.Context, host, service string) ([]CheckState, error)
	SetDowntimes(ctx context.Context, r Downtime) (string, error)
	GetDowntimes(ctx context.Context, r Downtime) ([]Downtime, error)
}

type Downtime struct {
	Description string
	Filters     []DowntimeFilter
	EndTime     time.Time
	DowntimeID  string
}

func NewSetDowntimeRequestByDuration(description string, duration time.Duration, filters []DowntimeFilter) Downtime {
	return Downtime{
		Description: description,
		Filters:     filters,
		EndTime:     time.Now().Add(duration),
	}
}

type DowntimeFilter struct {
	Host       string
	Namespace  string
	DowntimeID string
	Service    string
}
