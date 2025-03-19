package kfmodels

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/defaults"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/helpers"
)

func TestKafkaConfig_ValidateLogPreallocate(t *testing.T) {
	t.Run("When empty KafkaConfig then no error", func(t *testing.T) {
		config := KafkaConfig{}
		err := config.Validate()
		require.NoError(t, err)
	})

	t.Run("When LogPreallocate false then no error", func(t *testing.T) {
		config := KafkaConfig{
			LogPreallocate: helpers.Pointer[bool](false),
		}
		err := config.Validate()
		require.NoError(t, err)
	})

	t.Run("When LogPreallocate true then no error", func(t *testing.T) {
		config := KafkaConfig{
			LogPreallocate: helpers.Pointer[bool](true),
		}
		err := config.Validate()
		require.Error(t, err)
		require.Equal(t, "preallocate flag is disabled due the kafka issue KAFKA-13664", err.Error())
	})
}

func TestKafkaConfig_ValidateSslCipherSuites(t *testing.T) {
	t.Run("When all possible valid suites then no error", func(t *testing.T) {
		config := KafkaConfig{
			SslCipherSuites: defaults.AllValidSslCipherSuitesSortedSlice,
		}
		err := config.Validate()
		require.NoError(t, err)
	})

	t.Run("Single incorrect cipher suit then error", func(t *testing.T) {
		config := KafkaConfig{
			SslCipherSuites: []string{"blank"},
		}
		err := config.Validate()
		require.Error(t, err)
		require.Equal(t, fmt.Sprintf("these suites are invalid: [blank]. List of valid suites: [%s].", defaults.AllValidSslCipherSuitesSortedString), err.Error())
	})

	t.Run("Multiple incorrect cipher suites then error", func(t *testing.T) {
		config := KafkaConfig{
			SslCipherSuites: []string{"blank", "abc"},
		}
		err := config.Validate()
		require.Error(t, err)
		require.Equal(t, fmt.Sprintf("these suites are invalid: [abc blank]. List of valid suites: [%s].", defaults.AllValidSslCipherSuitesSortedString), err.Error())
	})

	t.Run("Multiple incorrect cipher suites with some corrects cipher suites then error", func(t *testing.T) {
		config := KafkaConfig{
			SslCipherSuites: []string{"blank", "abc", "TLS_AKE_WITH_AES_256_GCM_SHA384"},
		}
		err := config.Validate()
		require.Error(t, err)
		require.Equal(t, fmt.Sprintf("these suites are invalid: [abc blank]. List of valid suites: [%s].", defaults.AllValidSslCipherSuitesSortedString), err.Error())
	})

	t.Run("Single correct cipher suit then no error", func(t *testing.T) {
		config := KafkaConfig{
			SslCipherSuites: []string{"TLS_DHE_RSA_WITH_AES_128_CBC_SHA256"},
		}
		err := config.Validate()
		require.NoError(t, err)
	})

	t.Run("Single correct cipher suit with duplicate then no error", func(t *testing.T) {
		config := KafkaConfig{
			SslCipherSuites: []string{"TLS_DHE_RSA_WITH_AES_128_CBC_SHA256", "TLS_DHE_RSA_WITH_AES_128_CBC_SHA256"},
		}
		err := config.Validate()
		require.NoError(t, err)
	})

	t.Run("Multiple correct cipher suits then no error", func(t *testing.T) {
		config := KafkaConfig{
			SslCipherSuites: []string{"TLS_DHE_RSA_WITH_AES_128_CBC_SHA256", "TLS_AKE_WITH_CHACHA20_POLY1305_SHA256", "TLS_RSA_WITH_AES_256_CBC_SHA256"},
		}
		err := config.Validate()
		require.NoError(t, err)
	})
}

func TestKafkaConfig_FullKafkaConfig(t *testing.T) {
	t.Run("When full correct Kafka Config then no error", func(t *testing.T) {
		config := KafkaConfig{
			CompressionType:             CompressionTypeGzip,
			LogFlushIntervalMessages:    helpers.Pointer[int64](int64(1)),
			LogFlushIntervalMs:          helpers.Pointer[int64](int64(1)),
			LogFlushSchedulerIntervalMs: helpers.Pointer[int64](int64(1)),
			LogRetentionBytes:           helpers.Pointer[int64](int64(1)),
			LogRetentionHours:           helpers.Pointer[int64](int64(1)),
			LogRetentionMinutes:         helpers.Pointer[int64](int64(1)),
			LogRetentionMs:              helpers.Pointer[int64](int64(1)),
			LogSegmentBytes:             helpers.Pointer[int64](int64(1)),
			LogPreallocate:              helpers.Pointer[bool](false),
			SocketSendBufferBytes:       helpers.Pointer[int64](int64(1)),
			SocketReceiveBufferBytes:    helpers.Pointer[int64](int64(1)),
			AutoCreateTopicsEnable:      helpers.Pointer[bool](true),
			NumPartitions:               helpers.Pointer[int64](int64(1)),
			DefaultReplicationFactor:    helpers.Pointer[int64](int64(1)),
			MessageMaxBytes:             helpers.Pointer[int64](int64(1)),
			ReplicaFetchMaxBytes:        helpers.Pointer[int64](int64(1)),
			SslCipherSuites:             defaults.AllValidSslCipherSuitesSortedSlice,
			OffsetsRetentionMinutes:     helpers.Pointer[int64](int64(1)),
		}
		err := config.Validate()
		require.NoError(t, err)
	})

	t.Run("When full Kafka Config with invalid sslCipherSuites then error", func(t *testing.T) {
		config := KafkaConfig{
			CompressionType:             CompressionTypeGzip,
			LogFlushIntervalMessages:    helpers.Pointer[int64](int64(1)),
			LogFlushIntervalMs:          helpers.Pointer[int64](int64(1)),
			LogFlushSchedulerIntervalMs: helpers.Pointer[int64](int64(1)),
			LogRetentionBytes:           helpers.Pointer[int64](int64(1)),
			LogRetentionHours:           helpers.Pointer[int64](int64(1)),
			LogRetentionMinutes:         helpers.Pointer[int64](int64(1)),
			LogRetentionMs:              helpers.Pointer[int64](int64(1)),
			LogSegmentBytes:             helpers.Pointer[int64](int64(1)),
			LogPreallocate:              helpers.Pointer[bool](false),
			SocketSendBufferBytes:       helpers.Pointer[int64](int64(1)),
			SocketReceiveBufferBytes:    helpers.Pointer[int64](int64(1)),
			AutoCreateTopicsEnable:      helpers.Pointer[bool](true),
			NumPartitions:               helpers.Pointer[int64](int64(1)),
			DefaultReplicationFactor:    helpers.Pointer[int64](int64(1)),
			MessageMaxBytes:             helpers.Pointer[int64](int64(1)),
			ReplicaFetchMaxBytes:        helpers.Pointer[int64](int64(1)),
			SslCipherSuites:             []string{"blank"},
			OffsetsRetentionMinutes:     helpers.Pointer[int64](int64(1)),
		}
		err := config.Validate()
		require.Error(t, err)
		require.Equal(t, fmt.Sprintf("these suites are invalid: [blank]. List of valid suites: [%s].", defaults.AllValidSslCipherSuitesSortedString), err.Error())
	})
}

func TestKafkaConfig_KafkaConfigIsEmpty(t *testing.T) {
	t.Run("When empty KafkaConfig then true", func(t *testing.T) {
		require.True(t, KafkaConfigIsEmpty(KafkaConfig{}))
	})

	t.Run("When not empty KafkaConfig then false", func(t *testing.T) {
		require.False(t, KafkaConfigIsEmpty(KafkaConfig{SslCipherSuites: []string{"TLS_AKE_WITH_AES_256_GCM_SHA384"}}))
	})
}
