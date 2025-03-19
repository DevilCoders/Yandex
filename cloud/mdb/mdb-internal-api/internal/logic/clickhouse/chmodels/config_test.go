package chmodels

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
)

func TestMergeClickhouseConfigs(t *testing.T) {
	defaultMergeTree := MergeTreeConfig{
		EnableMixedGranularityParts:                    optional.NewBool(false),
		MaxBytesToMergeAtMinSpaceInPool:                optional.NewInt64(1048576),
		MaxReplicatedMergesInQueue:                     optional.NewInt64(16),
		NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge: optional.NewInt64(8),
		PartsToDelayInsert:                             optional.NewInt64(150),
		PartsToThrowInsert:                             optional.NewInt64(300),
		ReplicatedDeduplicationWindow:                  optional.NewInt64(100),
		ReplicatedDeduplicationWindowSeconds:           optional.NewInt64(604800),
	}

	userMergeTree := MergeTreeConfig{
		EnableMixedGranularityParts:          optional.NewBool(true),
		MaxBytesToMergeAtMaxSpaceInPool:      optional.NewInt64(200),
		InactivePartsToDelayInsert:           optional.NewInt64(10),
		ReplicatedDeduplicationWindow:        optional.NewInt64(10),
		ReplicatedDeduplicationWindowSeconds: optional.NewInt64(1),
	}

	effectiveMergeTree := MergeTreeConfig{
		EnableMixedGranularityParts:                    optional.NewBool(true),
		MaxBytesToMergeAtMinSpaceInPool:                optional.NewInt64(1048576),
		MaxBytesToMergeAtMaxSpaceInPool:                optional.NewInt64(200),
		MaxReplicatedMergesInQueue:                     optional.NewInt64(16),
		NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge: optional.NewInt64(8),
		PartsToDelayInsert:                             optional.NewInt64(150),
		PartsToThrowInsert:                             optional.NewInt64(300),
		InactivePartsToDelayInsert:                     optional.NewInt64(10),
		ReplicatedDeduplicationWindow:                  optional.NewInt64(10),
		ReplicatedDeduplicationWindowSeconds:           optional.NewInt64(1),
	}

	defaultKafka := Kafka{}
	userKafka := Kafka{
		SecurityProtocol: KafkaSecurityProtocolSSL,
		SaslMechanism:    KafkaSaslMechanismScramSHA512,
		SaslUsername:     optional.NewString("user"),
		SaslPassword:     optional.NewOptionalPassword(secret.NewString("")),
		Valid:            true,
	}
	effectiveKafka := userKafka

	defaultRabbitMQ := RabbitMQ{}
	userRabbitMQ := RabbitMQ{
		Username: optional.NewString("user"),
		Password: optional.NewOptionalPassword(secret.NewString("")),
		Valid:    true,
	}
	effectiveRabbitMQ := userRabbitMQ

	userCompressions := []Compression{{}}
	effectiveCompressions := userCompressions

	userDictionaries := []Dictionary{{}}
	effectiveDictionaries := userDictionaries

	userGraphiteRollups := []GraphiteRollup{{}}
	effectiveGraphiteRollups := userGraphiteRollups

	userKafkaTopics := []KafkaTopic{{}}
	effectiveKafkaTopics := userKafkaTopics

	defaultConfig := ClickHouseConfig{
		LogLevel:                          LogLevelDebug,
		MergeTree:                         defaultMergeTree,
		Kafka:                             defaultKafka,
		RabbitMQ:                          defaultRabbitMQ,
		QueryLogRetentionSize:             optional.NewInt64(1073741824),
		QueryLogRetentionTime:             optional.NewInt64(2592000),
		QueryThreadLogEnabled:             optional.NewBool(true),
		QueryThreadLogRetentionSize:       optional.NewInt64(5367870912),
		QueryThreadLogRetentionTime:       optional.NewInt64(2592000),
		PartLogRetentionSize:              optional.NewInt64(536870912),
		PartLogRetentionTime:              optional.NewInt64(2592000),
		MetricLogEnabled:                  optional.NewBool(true),
		MetricLogRetentionSize:            optional.NewInt64(536870912),
		MetricLogRetentionTime:            optional.NewInt64(2592000),
		TraceLogEnabled:                   optional.NewBool(true),
		TraceLogRetentionSize:             optional.NewInt64(536870912),
		TraceLogRetentionTime:             optional.NewInt64(2592000),
		TextLogEnabled:                    optional.NewBool(false),
		TextLogRetentionSize:              optional.NewInt64(536870912),
		TextLogRetentionTime:              optional.NewInt64(2592000),
		TextLogLevel:                      LogLevelTrace,
		BuiltinDictionariesReloadInterval: optional.NewInt64(3600),
		KeepAliveTimeout:                  optional.NewInt64(3),
		MarkCacheSize:                     optional.NewInt64(5368709120),
		MaxConcurrentQueries:              optional.NewInt64(500),
		MaxConnections:                    optional.NewInt64(4096),
		MaxPartitionSizeToDrop:            optional.NewInt64(53687091200),
		MaxTableSizeToDrop:                optional.NewInt64(53687091200),
		Timezone:                          optional.NewString("Europe/Moscow"),
		UncompressedCacheSize:             optional.NewInt64(8589934592),
	}
	userConfig := ClickHouseConfig{
		MergeTree:            userMergeTree,
		Compressions:         userCompressions,
		Dictionaries:         userDictionaries,
		GraphiteRollups:      userGraphiteRollups,
		Kafka:                userKafka,
		KafkaTopics:          userKafkaTopics,
		RabbitMQ:             userRabbitMQ,
		TextLogEnabled:       optional.NewBool(true),
		TextLogRetentionTime: optional.NewInt64(1000),
		TextLogLevel:         LogLevelDebug,
		GeobaseURI:           optional.NewString("a"),
	}
	effectiveConfig := ClickHouseConfig{
		LogLevel:                          LogLevelDebug,
		MergeTree:                         effectiveMergeTree,
		Compressions:                      effectiveCompressions,
		Dictionaries:                      effectiveDictionaries,
		GraphiteRollups:                   effectiveGraphiteRollups,
		Kafka:                             effectiveKafka,
		KafkaTopics:                       effectiveKafkaTopics,
		RabbitMQ:                          effectiveRabbitMQ,
		QueryLogRetentionSize:             optional.NewInt64(1073741824),
		QueryLogRetentionTime:             optional.NewInt64(2592000),
		QueryThreadLogEnabled:             optional.NewBool(true),
		QueryThreadLogRetentionSize:       optional.NewInt64(5367870912),
		QueryThreadLogRetentionTime:       optional.NewInt64(2592000),
		PartLogRetentionSize:              optional.NewInt64(536870912),
		PartLogRetentionTime:              optional.NewInt64(2592000),
		MetricLogEnabled:                  optional.NewBool(true),
		MetricLogRetentionSize:            optional.NewInt64(536870912),
		MetricLogRetentionTime:            optional.NewInt64(2592000),
		TraceLogEnabled:                   optional.NewBool(true),
		TraceLogRetentionSize:             optional.NewInt64(536870912),
		TraceLogRetentionTime:             optional.NewInt64(2592000),
		TextLogEnabled:                    optional.NewBool(true),
		TextLogRetentionSize:              optional.NewInt64(536870912),
		TextLogRetentionTime:              optional.NewInt64(1000),
		TextLogLevel:                      LogLevelDebug,
		BuiltinDictionariesReloadInterval: optional.NewInt64(3600),
		GeobaseURI:                        optional.NewString("a"),
		KeepAliveTimeout:                  optional.NewInt64(3),
		MarkCacheSize:                     optional.NewInt64(5368709120),
		MaxConcurrentQueries:              optional.NewInt64(500),
		MaxConnections:                    optional.NewInt64(4096),
		MaxPartitionSizeToDrop:            optional.NewInt64(53687091200),
		MaxTableSizeToDrop:                optional.NewInt64(53687091200),
		Timezone:                          optional.NewString("Europe/Moscow"),
		UncompressedCacheSize:             optional.NewInt64(8589934592),
	}

	t.Run("MergeTree", func(t *testing.T) {
		actual, err := mergeOptionalFieldsOfStructs(&defaultMergeTree, &userMergeTree)
		require.NoError(t, err)
		require.Equal(t, effectiveMergeTree, *actual.(*MergeTreeConfig))
	})

	t.Run("ClickhouseConfig", func(t *testing.T) {
		actual, err := MergeClickhouseConfigs(defaultConfig, userConfig)
		require.NoError(t, err)
		require.Equal(t, effectiveConfig, actual)
	})
}
