package logbroker

import (
	"crypto/tls"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	lblog "a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log"
)

type consumerConfig struct {
	persqueue.ReaderOptions

	Logger log.Logger

	skipBackPressure bool
	skipFatals       bool

	maxReadDuration time.Duration
	readLimit       int
	sizeLimit       int
}

// ConsumerOption represents portion of configuration for ConsumerService.
type ConsumerOption func(*consumerConfig)

// Credentials sets credentials to logbroker reader
func Credentials(c ydb.Credentials) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.Credentials = c
	}
}

// Database set database option of logbroker reader.
func Database(db string) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.Database = db
	}
}

// UseTLS forces to use TLS in logbroker reader. No futher configuration done.
func UseTLS() ConsumerOption {
	return func(cfg *consumerConfig) {
		if cfg.TLSConfig == nil {
			cfg.TLSConfig = &tls.Config{MinVersion: tls.VersionTLS12}
		}
	}
}

// TLS set configuration to logbroker reader.
func TLS(c *tls.Config) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.TLSConfig = c
	}
}

// Endpoint set host and port for logbroker reader.
func Endpoint(ep string, port int) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.Endpoint = ep
		cfg.Port = port
	}
}

// Logger set service and reader logger.
func Logger(l log.Logger) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.Logger = l
	}
}

// Logger set service and reader logger.
func ReaderLogger(l lblog.Logger) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.ReaderOptions.Logger = l
	}
}

// ReadNonLocal allow to read not only local topics by setting ReadOnlyLocal to false.
func ReadNonLocal() ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.ReadOnlyLocal = false
	}
}

// AutoRetry allows logbroker reader automatically retry all nonfatal errors.
// Use with caution.
func AutoRetry() ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.RetryOnFailure = true
	}
}

// ForceRebalance allow logbroker rebalance partitions without wait inflight messages to commit.
func ForceRebalance() ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.ForceRebalance = true
	}
}

// ConsumeTopic set name and topic to reader. No partition limitations applied.
func ConsumeTopic(name string, topic string) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.Consumer = name
		cfg.Topics = []persqueue.TopicInfo{{
			Topic: topic,
		}}
	}
}

// ConsumePartitions set name, topic and partition groups to reader. Use if service should control which
// partitions to read.
func ConsumePartitions(name string, topic string, partitionGroups []uint32) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.Consumer = name
		cfg.Topics = []persqueue.TopicInfo{{
			Topic:           topic,
			PartitionGroups: partitionGroups,
		}}
	}
}

// ConsumeLimits sets reader limits.
//  - messages set maximum messages count in one logbroker read
//  - size limits messages size in one low level read (not whole batches size)
//  - partitions limits simultaneous partitions for reading
//  - lag limits maximum messages lag for reader
//
func ConsumeLimits(messages uint32, size uint32, partitions uint32, lag time.Duration) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.MaxReadMessagesCount = messages
		cfg.MaxReadSize = size
		cfg.MaxReadPartitionsCount = partitions
		cfg.MaxTimeLag = lag
	}
}

// SDKDecompression controls if messages decompression should be processed by SDK during preprocessing.
// NOTE: if enabled all messages in infly buffers will be stored as decompressed data.
//
func SDKDecompression(v bool) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.DecompressionDisabled = !v
	}
}

// ReadLimits set limits to one service logical read.
//  - duration limits time to collect large batch of messages to process
//  - messages limits count of one handling loop
//
func ReadLimits(duration time.Duration, messages uint32, size int) ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.maxReadDuration = duration
		cfg.readLimit = int(messages)
		cfg.sizeLimit = size
	}
}

// NoBackPressure sets service to not suspend in case of errors and try to restart reader at once.
func NoBackPressure() ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.skipBackPressure = true
	}
}

// SkipFatals forces service to ignore fatal reader errors.
func SkipFatals() ConsumerOption {
	return func(cfg *consumerConfig) {
		cfg.skipFatals = true
	}
}

func defaultConfig(cfg *consumerConfig) {
	cfg.ReadOnlyLocal = true
	cfg.ManualPartitionAssignment = true
	cfg.DecompressionDisabled = true
	cfg.MaxTimeLag = time.Hour * 24 * 13
}
