package uazap

import (
	"sync"

	uaclient "a.yandex-team.ru/library/go/yandex/uagent/log/zap/client"
)

type Stat struct {
	// ReceivedMessages is a total number of logs
	// which were successfully encoded by zap.Encoder (JSONEncoder never returns error).
	ReceivedMessages int64

	// ReceivedBytes is a total number of bytes in logs including metadata.
	// which were successfully encoded by zap.Encoder
	ReceivedBytes int64

	// InflightMessages is a number of logs which were sent to client but not yet ack'ed.
	// Basically equals to ReceivedMessages - DroppedMessages - ack'ed messages.
	InflightMessages int64

	// InflightBytes is a number of bytes in logs including metadata which were sent to client but not yet ack'ed.
	// Basically equals to ReceivedBytes - DroppedBytes - ack'ed bytes.
	InflightBytes int64

	// DroppedMessages is a number of logs which were dropped due to
	// GRPCMessageMaxSize exceeded, rate limited, in case message causes MaxMemoryUsage overflow or if core is stopped.
	DroppedMessages int64

	// DroppedBytes is a number of bytes in logs including metadata which were dropped due to
	// GRPCMessageMaxSize exceeded, rate limited, in case message causes MaxMemoryUsage overflow or if core is stopped.
	DroppedBytes int64

	// ErrorsCount is a number of DroppedMessages and client errors.
	ErrorsCount int64

	mu sync.Mutex
}

func (s *Stat) receiveMessage(msgSize int64) {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.ReceivedMessages++
	s.ReceivedBytes += msgSize
}

func (s *Stat) inflightMessageLocked(msgSize int64) {
	s.InflightMessages++
	s.InflightBytes += msgSize
}

func (s *Stat) dropInflightMessages(messages []uaclient.Message) {
	s.mu.Lock()
	defer s.mu.Unlock()

	for _, msg := range messages {
		s.InflightMessages--
		s.InflightBytes -= msg.Size()
	}
}

func (s *Stat) dropMessage(msgSize int64) {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.dropMessageLocked(msgSize)
}

func (s *Stat) dropMessageLocked(msgSize int64) {
	s.DroppedMessages++
	s.DroppedBytes += msgSize
}
