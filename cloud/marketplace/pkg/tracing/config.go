package tracing

import "time"

type Config struct {
	ServiceName string

	LocalAgentHostPort  string
	BufferFlushInterval time.Duration
	QueueSize           int
}
