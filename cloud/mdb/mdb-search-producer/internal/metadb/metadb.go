package metadb

import (
	"context"
	"io"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

// UnsentSearchDoc ...
type UnsentSearchDoc struct {
	QueueID   int64
	Doc       string
	CreatedAt time.Time
}

// NonEnumeratedSearchDoc is a search document without QueueID
type NonEnumeratedSearchDoc struct {
	ID        int64
	CreatedAt time.Time
}
type OnSearchQueueEventsHandler func(context.Context, []UnsentSearchDoc) ([]int64, error)

//go:generate ../../../scripts/mockgen.sh MetaDB

// MetaDB is an search-producer interface to MetaDB
type MetaDB interface {
	io.Closer
	ready.Checker

	// NonEnumeratedSearchDocs return limit NonEnumeratedSearchDocs
	NonEnumeratedSearchDocs(ctx context.Context, limit int64) ([]NonEnumeratedSearchDoc, error)

	// UnsentSearchDocs return limit UnsentSearchDocs
	UnsentSearchDocs(ctx context.Context, limit int64) ([]UnsentSearchDoc, error)

	// OnUnsentWorkerQueueStartEvents call handler on worker_queue_events that has unset start_sent.
	// Lock events before call workers on it.
	// Mark as sent events that handler returns.
	OnUnsentSearchQueueDoc(ctx context.Context, limit int64, handler OnSearchQueueEventsHandler) (int, error)
}
