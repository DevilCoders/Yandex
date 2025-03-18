package uaclient

import (
	"context"
	"time"
	"unsafe"

	"a.yandex-team.ru/library/go/core/metrics"
)

//go:generate ya tool mockgen -source ua.go -package mock -destination mock/client.go

type Client interface {
	Send(clientMessages []Message) error

	Stat() Stats

	GRPCMaxMessageSize() int64

	CollectMetrics(r metrics.Registry, c metrics.CollectPolicy)

	Close(ctx context.Context) error
}

type Stats interface {
	AckedBytes() int64
	AckedMessages() int64

	DroppedBytes() int64
	DroppedMessages() int64

	ErrorCount() int64
}

// [BEGIN message definition]

// Message is the message to be sent to unified agent.
type Message struct {
	// Payload is the payload of the message.
	Payload []byte

	// Meta is message metadata as key-value set.
	// Can be used by agent filters and outputs for validation/routing/enrichment/etc.
	Meta map[string]string

	// Time is message timestamp.
	// Default: time the client library has received the instance of ClientMessage.
	Time *time.Time

	// seqNo represents message sequence number.
	// seqNo is set in the Client when message is added to the deque in Send method.
	seqNo int64
}

// [END message definition]

// timestampSize is the size of the time variable.
var timestampSize = int64(unsafe.Sizeof(time.Time{}))

func (c *Message) SetSeqNo(seqNo int64) {
	c.seqNo = seqNo
}

func (c *Message) GetSeqNo() int64 {
	return c.seqNo
}

// Size returns size of the message in bytes.
// Size consists of Payload size, Meta size (not compressed), and Time variable size.
func (c *Message) Size() int64 {
	metaSize := int64(len(c.Payload))
	metaSize += timestampSize
	for k, v := range c.Meta {
		metaSize += int64(len(k)) + int64(len(v))
	}
	return metaSize
}

// UnixMicro returns ClientMessage time in microseconds.
func (c *Message) UnixMicro() uint64 {
	return uint64(c.Time.UnixNano() / 1_000)
}
