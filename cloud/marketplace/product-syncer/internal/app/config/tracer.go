package config

import "time"

type Tracer struct {
	LocalAgentHostPort  string        `config:"agent-endpoint" yaml:"agent-endpoint"`
	BufferFlushInterval time.Duration `config:"flush-interval" yaml:"flush-interval"`
	QueueSize           int           `config:"queue-size" yaml:"queue-size"`
}
