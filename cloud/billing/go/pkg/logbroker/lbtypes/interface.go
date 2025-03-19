package lbtypes

import (
	"context"
	"io"
	"time"
)

// ShardProducer is logbroker producer for recording data to multiple partitions with
// offset save logic.
type ShardProducer interface {
	OffsetReporter
	ShardWriter
}

// OffsetReporter can return stored offset by source id
type OffsetReporter interface {
	GetOffset(context.Context, SourceID) (uint64, error)
}

// ShardWriter can write partitioned data to logbroker
type ShardWriter interface {
	Write(ctx context.Context, srcID SourceID, partition uint32, messages []ShardMessage) (writtenOffset uint64, err error)
	PartitionsCount() uint32
}

// ShardMessage is combination of message data and offset from it's source.
// Messages will be group by such manner that messages with same offset will be pushed in one message.
type ShardMessage interface {
	Data() ([]byte, error)
	Offset() uint64
}

// Handler process logbroker messages
type Handler interface {
	Handle(context.Context, SourceID, *Messages)
}

// HandlerFunc makes Handler from function
type HandlerFunc func(context.Context, SourceID, *Messages)

func (h HandlerFunc) Handle(ctx context.Context, src SourceID, m *Messages) {
	h(ctx, src, m)
}

// ReadCommittedOffsets is offset reporter that allows read all messages from last committed offset.
var ReadCommittedOffsets OffsetReporter = dummyOffsets{}

// SourceID is type for logbroker source identification
type SourceID string

type DataReader interface {
	io.ReadCloser
	RawSize() int
}

// ReadMessage is struct describing incoming message. Copy of persqueue.ReadMessage but without dependencies.
type ReadMessage struct {
	Offset   uint64
	SeqNo    uint64
	SourceID []byte

	CreateTime time.Time
	WriteTime  time.Time
	IP         string

	DataReader
	// Codec Codec

	// ExtraFields map[string]string
}

// Messages is struct for pass data and processing callbacks to Handler
type Messages struct {
	MessagesReporter

	Messages      []ReadMessage
	LastOffset    uint64
	LastWriteTime time.Time
}

type MessagesReporter interface {
	Consumed()
	Error(error)
}

// Stat is struct shows logbroker reader usage statistic. Copy from persqueue without dependencies
type Stat struct {
	// MemUsage is amount of RAM in bytes currently in flight (read with no ack).
	MemUsage int
	// InflightCount is count of message currently in flight.
	InflightCount int
	// WaitAckCount is count of messages that mark as read at client but no ack
	WaitAckCount int

	// BytesExtracted growing counter of extracted bytes by reader
	BytesExtracted uint64
	// BytesRead growing counter of raw read bytes by reader
	BytesRead uint64
	SessionID string
}

// ServiceCounters stores service statistics and status
type ServiceCounters struct {
	Status ServiceStatus

	InflyMessages int
	ReadMessages  int

	HandleCalls         int
	Handled             int
	HandleFailures      int
	HandleCancellations int

	Locks       int
	ActiveLocks int
	LastEventAt time.Time

	Restarts int
	Suspends int

	ReaderStats Stat
}

// ServiceStatus represents current service status in statistics
type ServiceStatus string

const (
	ServiceNotRunning = "not_running"
	ServiceRunning    = "running"
	ServiceSuspended  = "suspended"
	ServiceStarting   = "starting"
	ServiceSuspending = "suspending"
	ServiceShutdown   = "shutdown"
	ServiceError      = "error"
)

type dummyOffsets struct{}

func (dummyOffsets) GetOffset(context.Context, SourceID) (uint64, error) {
	return 0, nil
}
