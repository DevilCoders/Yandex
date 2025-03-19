package storage

import "time"

// ListRequest is the criteria to match.
type ListRequest struct {
	N             int64
	Last          string
	Disk          string
	Project       string
	BillingStart  *time.Time
	BillingEnd    *time.Time
	SearchPrefix  string
	SearchFull    string
	CreatedAfter  *time.Time
	CreatedBefore *time.Time
	Sort          string
}

// BaseListRequest is a request for ListBases.
type BaseListRequest struct {
	N  int64
	ID string
}

// GCListRequest defines the criteria for snapshots to match to be garbage-collected.
type GCListRequest struct {
	N                int
	FailedCreation   time.Duration
	FailedConversion time.Duration
	FailedDeletion   time.Duration
	Tombstone        time.Duration
}
