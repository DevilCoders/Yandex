package clickhouse

import (
	"context"
	"time"

	"github.com/jmoiron/sqlx"
)

type Cluster interface {
	DB(partition int) (*sqlx.DB, error)
	Partitions() int
}

type Adapter struct {
	runCtx          context.Context
	numOfPartitions int
	connections     Cluster
	splitter        splitter
}

func New(runCtx context.Context, connections Cluster) (*Adapter, error) {
	numOfPartitions := connections.Partitions()
	adapter := &Adapter{
		runCtx:          runCtx,
		numOfPartitions: numOfPartitions,
		connections:     connections,
		splitter:        newSplitter(numOfPartitions),
	}
	return adapter, adapter.init()
}

func (a *Adapter) init() error {
	return nil
}

type Session struct {
	retrier
	adapter                   *Adapter
	invalidBufferForPartition []invalidStore
	nowOverride               func() time.Time
}

func (a *Adapter) Session() *Session {
	return &Session{
		adapter: a,
	}
}

func (s *Session) now() time.Time {
	if s.nowOverride != nil {
		return s.nowOverride()
	}
	return time.Now()
}
