package clickhouse

import (
	"google.golang.org/genproto/googleapis/type/timeofday"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	config "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1/config"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

func ClusterConfigToGRPC(cc chmodels.ClusterConfig) *chv1.ClusterConfig {
	return &chv1.ClusterConfig{
		Version: cc.Version,
		Clickhouse: &chv1.ClusterConfig_Clickhouse{
			Config: &config.ClickhouseConfigSet{
				EffectiveConfig: ClickhouseConfigToGRPC(cc.ClickhouseConfigSet.Effective),
				UserConfig:      ClickhouseConfigToGRPC(cc.ClickhouseConfigSet.User),
				DefaultConfig:   ClickhouseConfigToGRPC(cc.ClickhouseConfigSet.Default),
			},
			Resources: ClusterResourcesToGRPC(cc.ClickhouseResources),
		},
		Zookeeper: &chv1.ClusterConfig_Zookeeper{
			Resources: ClusterResourcesToGRPC(cc.ZooKeeperResources),
		},
		BackupWindowStart: &timeofday.TimeOfDay{
			Hours:   int32(cc.BackupWindowStart.Hours),
			Minutes: int32(cc.BackupWindowStart.Minutes),
			Seconds: int32(cc.BackupWindowStart.Seconds),
			Nanos:   int32(cc.BackupWindowStart.Nanos),
		},
		Access: AccessToGRPC(cc.Access),
		CloudStorage: &chv1.CloudStorage{
			Enabled:          cc.CloudStorageEnabled,
			DataCacheEnabled: grpc.OptionalBoolToGRPC(cc.CloudStorageConfig.DataCacheEnabled),
			DataCacheMaxSize: grpc.OptionalInt64ToGRPC(cc.CloudStorageConfig.DataCacheMaxSize),
			MoveFactor:       grpc.OptionalFloat64ToGRPC(cc.CloudStorageConfig.MoveFactor),
		},
		SqlDatabaseManagement: grpc.OptionalBoolToGRPC(optional.NewBool(cc.SQLDatabaseManagement)),
		SqlUserManagement:     grpc.OptionalBoolToGRPC(optional.NewBool(cc.SQLUserManagement)),
		EmbeddedKeeper:        grpc.OptionalBoolToGRPC(optional.NewBool(cc.EmbeddedKeeper)),
		MysqlProtocol:         grpc.OptionalBoolToGRPC(optional.NewBool(cc.MySQLProtocol)),
		PostgresqlProtocol:    grpc.OptionalBoolToGRPC(optional.NewBool(cc.PostgreSQLProtocol)),
	}
}

func AccessToGRPC(a clusters.Access) *chv1.Access {
	return &chv1.Access{
		DataLens:     a.DataLens.Valid && a.DataLens.Bool,
		WebSql:       a.WebSQL.Valid && a.WebSQL.Bool,
		Metrika:      a.Metrica.Valid && a.Metrica.Bool,
		Serverless:   a.Serverless.Valid && a.Serverless.Bool,
		DataTransfer: a.DataTransfer.Valid && a.DataTransfer.Bool,
		YandexQuery:  a.YandexQuery.Valid && a.YandexQuery.Bool,
	}
}

func ClusterResourcesToGRPC(resources models.ClusterResources) *chv1.Resources {
	return &chv1.Resources{
		ResourcePresetId: resources.ResourcePresetExtID,
		DiskSize:         resources.DiskSize,
		DiskTypeId:       resources.DiskTypeExtID,
	}
}

// ClickHouse Config model mapping

func ClickhouseConfigToGRPC(cfg chmodels.ClickHouseConfig) *config.ClickhouseConfig {
	return &config.ClickhouseConfig{
		LogLevel:                          LogLevelToGRPC(cfg.LogLevel),
		MergeTree:                         MergeTreeToGRPC(cfg.MergeTree),
		Compression:                       CompressionsToGRPC(cfg.Compressions),
		Dictionaries:                      DictionariesToGRPC(cfg.Dictionaries),
		GraphiteRollup:                    GraphiteRollupsToGRPC(cfg.GraphiteRollups),
		Kafka:                             KafkaToGRPC(cfg.Kafka),
		KafkaTopics:                       KafkaTopicsToGRPC(cfg.KafkaTopics),
		Rabbitmq:                          RabbitMQToGRPC(cfg.RabbitMQ),
		MaxConnections:                    grpc.OptionalInt64ToGRPC(cfg.MaxConnections),
		MaxConcurrentQueries:              grpc.OptionalInt64ToGRPC(cfg.MaxConcurrentQueries),
		KeepAliveTimeout:                  grpc.OptionalInt64ToGRPC(cfg.KeepAliveTimeout),
		UncompressedCacheSize:             grpc.OptionalInt64ToGRPC(cfg.UncompressedCacheSize),
		MarkCacheSize:                     grpc.OptionalInt64ToGRPC(cfg.MarkCacheSize),
		MaxTableSizeToDrop:                grpc.OptionalInt64ToGRPC(cfg.MaxTableSizeToDrop),
		MaxPartitionSizeToDrop:            grpc.OptionalInt64ToGRPC(cfg.MaxPartitionSizeToDrop),
		BuiltinDictionariesReloadInterval: grpc.OptionalInt64ToGRPC(cfg.BuiltinDictionariesReloadInterval),
		Timezone:                          cfg.Timezone.String,
		GeobaseUri:                        cfg.GeobaseURI.String,
		QueryLogRetentionSize:             grpc.OptionalInt64ToGRPC(cfg.QueryLogRetentionSize),
		QueryLogRetentionTime:             grpc.OptionalSecondsToGRPCMilliseconds(cfg.QueryLogRetentionTime),
		QueryThreadLogEnabled:             grpc.OptionalBoolToGRPC(cfg.QueryThreadLogEnabled),
		QueryThreadLogRetentionSize:       grpc.OptionalInt64ToGRPC(cfg.QueryThreadLogRetentionSize),
		QueryThreadLogRetentionTime:       grpc.OptionalSecondsToGRPCMilliseconds(cfg.QueryThreadLogRetentionTime),
		PartLogRetentionSize:              grpc.OptionalInt64ToGRPC(cfg.PartLogRetentionSize),
		PartLogRetentionTime:              grpc.OptionalSecondsToGRPCMilliseconds(cfg.PartLogRetentionTime),
		MetricLogEnabled:                  grpc.OptionalBoolToGRPC(cfg.MetricLogEnabled),
		MetricLogRetentionSize:            grpc.OptionalInt64ToGRPC(cfg.MetricLogRetentionSize),
		MetricLogRetentionTime:            grpc.OptionalSecondsToGRPCMilliseconds(cfg.MetricLogRetentionTime),
		TraceLogEnabled:                   grpc.OptionalBoolToGRPC(cfg.TraceLogEnabled),
		TraceLogRetentionSize:             grpc.OptionalInt64ToGRPC(cfg.TraceLogRetentionSize),
		TraceLogRetentionTime:             grpc.OptionalSecondsToGRPCMilliseconds(cfg.TraceLogRetentionTime),
		TextLogEnabled:                    grpc.OptionalBoolToGRPC(cfg.TextLogEnabled),
		TextLogRetentionSize:              grpc.OptionalInt64ToGRPC(cfg.TextLogRetentionSize),
		TextLogRetentionTime:              grpc.OptionalSecondsToGRPCMilliseconds(cfg.TextLogRetentionTime),
		TextLogLevel:                      LogLevelToGRPC(cfg.TextLogLevel),
		BackgroundPoolSize:                grpc.OptionalInt64ToGRPC(cfg.BackgroundPoolSize),
		BackgroundSchedulePoolSize:        grpc.OptionalInt64ToGRPC(cfg.BackgroundSchedulePoolSize),
	}
}

func ClickHouseConfigFromGRPC(cfg *config.ClickhouseConfig) (chmodels.ClickHouseConfig, error) {
	if cfg == nil {
		return chmodels.ClickHouseConfig{}, nil
	}

	compressions, err := CompressionsFromGRPC(cfg.GetCompression())
	if err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	dictionaries, err := DictionariesFromGRPC(cfg.GetDictionaries())
	if err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	kafka, err := KafkaFromGRPC(cfg.GetKafka())
	if err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	kafkaTopics, err := KafkaTopicsFromGRPC(cfg.GetKafkaTopics())
	if err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	logLevel, err := LogLevelFromGRPC(cfg.GetLogLevel())
	if err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	textLogLevel, err := LogLevelFromGRPC(cfg.GetTextLogLevel())
	if err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	return chmodels.ClickHouseConfig{
		MergeTree: MergeTreeFromGRPC(cfg.GetMergeTree()),

		Compressions:    compressions,
		Dictionaries:    dictionaries,
		GraphiteRollups: GraphiteRollupsFromGRPC(cfg.GetGraphiteRollup()),
		Kafka:           kafka,
		KafkaTopics:     kafkaTopics,
		RabbitMQ:        RabbitMQFromGRPC(cfg.GetRabbitmq()),

		LogLevel:                          logLevel,
		MaxConnections:                    grpc.OptionalInt64FromGRPC(cfg.GetMaxConnections()),
		MaxConcurrentQueries:              grpc.OptionalInt64FromGRPC(cfg.GetMaxConcurrentQueries()),
		KeepAliveTimeout:                  grpc.OptionalInt64FromGRPC(cfg.GetKeepAliveTimeout()),
		UncompressedCacheSize:             grpc.OptionalInt64FromGRPC(cfg.GetUncompressedCacheSize()),
		MarkCacheSize:                     grpc.OptionalInt64FromGRPC(cfg.GetMarkCacheSize()),
		MaxTableSizeToDrop:                grpc.OptionalInt64FromGRPC(cfg.GetMaxTableSizeToDrop()),
		MaxPartitionSizeToDrop:            grpc.OptionalInt64FromGRPC(cfg.GetMaxPartitionSizeToDrop()),
		BuiltinDictionariesReloadInterval: grpc.OptionalInt64FromGRPC(cfg.GetBuiltinDictionariesReloadInterval()),
		Timezone:                          grpc.OptionalStringFromGRPC(cfg.GetTimezone()),
		GeobaseURI:                        grpc.OptionalStringFromGRPC(cfg.GetGeobaseUri()),
		QueryLogRetentionSize:             grpc.OptionalInt64FromGRPC(cfg.GetQueryLogRetentionSize()),
		QueryLogRetentionTime:             grpc.OptionalSecondsFromGRPCMilliseconds(cfg.GetQueryLogRetentionTime()),
		QueryThreadLogEnabled:             grpc.OptionalBoolFromGRPC(cfg.GetQueryThreadLogEnabled()),
		QueryThreadLogRetentionSize:       grpc.OptionalInt64FromGRPC(cfg.GetQueryThreadLogRetentionSize()),
		QueryThreadLogRetentionTime:       grpc.OptionalSecondsFromGRPCMilliseconds(cfg.GetQueryThreadLogRetentionTime()),
		PartLogRetentionSize:              grpc.OptionalInt64FromGRPC(cfg.GetPartLogRetentionSize()),
		PartLogRetentionTime:              grpc.OptionalSecondsFromGRPCMilliseconds(cfg.GetPartLogRetentionTime()),
		MetricLogEnabled:                  grpc.OptionalBoolFromGRPC(cfg.GetMetricLogEnabled()),
		MetricLogRetentionSize:            grpc.OptionalInt64FromGRPC(cfg.GetMetricLogRetentionSize()),
		MetricLogRetentionTime:            grpc.OptionalSecondsFromGRPCMilliseconds(cfg.GetMetricLogRetentionTime()),
		TraceLogEnabled:                   grpc.OptionalBoolFromGRPC(cfg.GetTraceLogEnabled()),
		TraceLogRetentionSize:             grpc.OptionalInt64FromGRPC(cfg.GetTraceLogRetentionSize()),
		TraceLogRetentionTime:             grpc.OptionalSecondsFromGRPCMilliseconds(cfg.GetTraceLogRetentionTime()),
		TextLogEnabled:                    grpc.OptionalBoolFromGRPC(cfg.GetTextLogEnabled()),
		TextLogRetentionSize:              grpc.OptionalInt64FromGRPC(cfg.GetTextLogRetentionSize()),
		TextLogRetentionTime:              grpc.OptionalSecondsFromGRPCMilliseconds(cfg.GetTextLogRetentionTime()),
		TextLogLevel:                      textLogLevel,
		BackgroundPoolSize:                grpc.OptionalInt64FromGRPC(cfg.GetBackgroundPoolSize()),
		BackgroundSchedulePoolSize:        grpc.OptionalInt64FromGRPC(cfg.GetBackgroundSchedulePoolSize()),
	}, nil
}

// Log level mapping

var (
	maplogLevelToGRPC = map[chmodels.ClickHouseLogLevel]config.ClickhouseConfig_LogLevel{
		chmodels.LogLevelUnknown:     config.ClickhouseConfig_LOG_LEVEL_UNSPECIFIED,
		chmodels.LogLevelTrace:       config.ClickhouseConfig_TRACE,
		chmodels.LogLevelDebug:       config.ClickhouseConfig_DEBUG,
		chmodels.LogLevelInformation: config.ClickhouseConfig_INFORMATION,
		chmodels.LogLevelWarning:     config.ClickhouseConfig_WARNING,
		chmodels.LogLevelError:       config.ClickhouseConfig_ERROR,
	}

	maplogLevelFromGRPC = reflectutil.ReverseMap(maplogLevelToGRPC).(map[config.ClickhouseConfig_LogLevel]chmodels.ClickHouseLogLevel)
)

func LogLevelToGRPC(l chmodels.ClickHouseLogLevel) config.ClickhouseConfig_LogLevel {
	v, ok := maplogLevelToGRPC[l]
	if !ok {
		return config.ClickhouseConfig_LOG_LEVEL_UNSPECIFIED
	}

	return v
}

func LogLevelFromGRPC(l config.ClickhouseConfig_LogLevel) (chmodels.ClickHouseLogLevel, error) {
	res, ok := maplogLevelFromGRPC[l]
	if !ok {
		return chmodels.LogLevelUnknown, semerr.InvalidInputf("invalid log_level: %q", l.String())
	}

	return res, nil
}

// Merge Tree mapping

func MergeTreeToGRPC(mt chmodels.MergeTreeConfig) *config.ClickhouseConfig_MergeTree {
	return &config.ClickhouseConfig_MergeTree{
		ReplicatedDeduplicationWindow:                  grpc.OptionalInt64ToGRPC(mt.ReplicatedDeduplicationWindow),
		ReplicatedDeduplicationWindowSeconds:           grpc.OptionalInt64ToGRPC(mt.ReplicatedDeduplicationWindowSeconds),
		PartsToDelayInsert:                             grpc.OptionalInt64ToGRPC(mt.PartsToDelayInsert),
		PartsToThrowInsert:                             grpc.OptionalInt64ToGRPC(mt.PartsToThrowInsert),
		MaxReplicatedMergesInQueue:                     grpc.OptionalInt64ToGRPC(mt.MaxReplicatedMergesInQueue),
		NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge: grpc.OptionalInt64ToGRPC(mt.NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge),
		MaxBytesToMergeAtMinSpaceInPool:                grpc.OptionalInt64ToGRPC(mt.MaxBytesToMergeAtMinSpaceInPool),
		MaxBytesToMergeAtMaxSpaceInPool:                grpc.OptionalInt64ToGRPC(mt.MaxBytesToMergeAtMaxSpaceInPool),
		EnableMixedGranularityParts:                    grpc.OptionalBoolToGRPC(mt.EnableMixedGranularityParts),
	}
}

func MergeTreeFromGRPC(cfg *config.ClickhouseConfig_MergeTree) chmodels.MergeTreeConfig {
	if cfg == nil {
		return chmodels.MergeTreeConfig{}
	}

	return chmodels.MergeTreeConfig{
		ReplicatedDeduplicationWindow:                  grpc.OptionalInt64FromGRPC(cfg.GetReplicatedDeduplicationWindow()),
		ReplicatedDeduplicationWindowSeconds:           grpc.OptionalInt64FromGRPC(cfg.GetReplicatedDeduplicationWindowSeconds()),
		PartsToDelayInsert:                             grpc.OptionalInt64FromGRPC(cfg.GetPartsToDelayInsert()),
		PartsToThrowInsert:                             grpc.OptionalInt64FromGRPC(cfg.GetPartsToThrowInsert()),
		MaxReplicatedMergesInQueue:                     grpc.OptionalInt64FromGRPC(cfg.GetMaxReplicatedMergesInQueue()),
		NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge: grpc.OptionalInt64FromGRPC(cfg.GetNumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge()),
		MaxBytesToMergeAtMinSpaceInPool:                grpc.OptionalInt64FromGRPC(cfg.GetMaxBytesToMergeAtMinSpaceInPool()),
		MaxBytesToMergeAtMaxSpaceInPool:                grpc.OptionalInt64FromGRPC(cfg.GetMaxBytesToMergeAtMaxSpaceInPool()),
		// Private fields.
		EnableMixedGranularityParts: grpc.OptionalBoolFromGRPC(cfg.GetEnableMixedGranularityParts()),
	}
}

// Compression config mapping

var (
	mapCompressionMethodFromGRPC = map[config.ClickhouseConfig_Compression_Method]chmodels.CompressionMethod{
		config.ClickhouseConfig_Compression_LZ4:  chmodels.CompressionMethodLZ4,
		config.ClickhouseConfig_Compression_ZSTD: chmodels.CompressionMethodZSTD,
	}

	mapCompressionMethodToGRPC = reflectutil.ReverseMap(mapCompressionMethodFromGRPC).(map[chmodels.CompressionMethod]config.ClickhouseConfig_Compression_Method)
)

func CompressionMethodToGRPC(method chmodels.CompressionMethod) config.ClickhouseConfig_Compression_Method {
	v, ok := mapCompressionMethodToGRPC[method]
	if !ok {
		return config.ClickhouseConfig_Compression_METHOD_UNSPECIFIED
	}

	return v
}

func CompressionMethodFromGRPC(method config.ClickhouseConfig_Compression_Method) (chmodels.CompressionMethod, error) {
	v, ok := mapCompressionMethodFromGRPC[method]
	if !ok {
		return chmodels.CompressionMethodLZ4, semerr.InvalidInputf("unknown compression method %q", method.String())
	}

	return v, nil
}

func CompressionFromGRPC(cs *config.ClickhouseConfig_Compression) (chmodels.Compression, error) {
	m, err := CompressionMethodFromGRPC(cs.GetMethod())
	if err != nil {
		return chmodels.Compression{}, err
	}

	return chmodels.Compression{
		Method:           m,
		MinPartSize:      cs.GetMinPartSize(),
		MinPartSizeRatio: cs.GetMinPartSizeRatio(),
	}, nil
}

func CompressionsFromGRPC(cs []*config.ClickhouseConfig_Compression) ([]chmodels.Compression, error) {
	res := make([]chmodels.Compression, 0, len(cs))
	for _, c := range cs {
		c, err := CompressionFromGRPC(c)
		if err != nil {
			return nil, err
		}
		res = append(res, c)
	}

	return res, nil
}

func CompressionToGRPC(c chmodels.Compression) *config.ClickhouseConfig_Compression {
	return &config.ClickhouseConfig_Compression{
		Method:           CompressionMethodToGRPC(c.Method),
		MinPartSize:      c.MinPartSize,
		MinPartSizeRatio: c.MinPartSizeRatio,
	}
}

func CompressionsToGRPC(cs []chmodels.Compression) []*config.ClickhouseConfig_Compression {
	res := make([]*config.ClickhouseConfig_Compression, len(cs))
	for i, c := range cs {
		res[i] = CompressionToGRPC(c)
	}
	return res
}

// Kafka configs mapping

var (
	mapKafkaSecurityProtocolFromGRPC = map[config.ClickhouseConfig_Kafka_SecurityProtocol]chmodels.KafkaSecurityProtocol{
		config.ClickhouseConfig_Kafka_SECURITY_PROTOCOL_UNSPECIFIED:    chmodels.KafkaSecurityProtocolUnspecified,
		config.ClickhouseConfig_Kafka_SECURITY_PROTOCOL_PLAINTEXT:      chmodels.KafkaSecurityProtocolPlainText,
		config.ClickhouseConfig_Kafka_SECURITY_PROTOCOL_SSL:            chmodels.KafkaSecurityProtocolSSL,
		config.ClickhouseConfig_Kafka_SECURITY_PROTOCOL_SASL_PLAINTEXT: chmodels.KafkaSecurityProtocolSaslPlainText,
		config.ClickhouseConfig_Kafka_SECURITY_PROTOCOL_SASL_SSL:       chmodels.KafkaSecurityProtocolSaslSSL,
	}
	mapKafkaSecurityProtocolToGRPC = reflectutil.ReverseMap(mapKafkaSecurityProtocolFromGRPC).(map[chmodels.KafkaSecurityProtocol]config.ClickhouseConfig_Kafka_SecurityProtocol)
)

func KafkaSecurityProtocolToGRPC(protocol chmodels.KafkaSecurityProtocol) config.ClickhouseConfig_Kafka_SecurityProtocol {
	v, ok := mapKafkaSecurityProtocolToGRPC[protocol]
	if !ok {
		return config.ClickhouseConfig_Kafka_SECURITY_PROTOCOL_UNSPECIFIED
	}

	return v
}

func KafkaSecurityProtocolFromGRPC(protocol config.ClickhouseConfig_Kafka_SecurityProtocol) (chmodels.KafkaSecurityProtocol, error) {
	v, ok := mapKafkaSecurityProtocolFromGRPC[protocol]
	if !ok {
		return chmodels.KafkaSecurityProtocolUnspecified, semerr.InvalidInputf("unknown kafka security protocol %q", protocol.String())
	}

	return v, nil
}

var (
	mapKafkaSaslMechanismFromGRPC = map[config.ClickhouseConfig_Kafka_SaslMechanism]chmodels.KafkaSaslMechanism{
		config.ClickhouseConfig_Kafka_SASL_MECHANISM_UNSPECIFIED:   chmodels.KafkaSaslMechanismUnspecified,
		config.ClickhouseConfig_Kafka_SASL_MECHANISM_GSSAPI:        chmodels.KafkaSaslMechanismGssAPI,
		config.ClickhouseConfig_Kafka_SASL_MECHANISM_PLAIN:         chmodels.KafkaSaslMechanismPlain,
		config.ClickhouseConfig_Kafka_SASL_MECHANISM_SCRAM_SHA_256: chmodels.KafkaSaslMechanismScramSHA256,
		config.ClickhouseConfig_Kafka_SASL_MECHANISM_SCRAM_SHA_512: chmodels.KafkaSaslMechanismScramSHA512,
	}
	mapKafkaSaslMechanismToGRPC = reflectutil.ReverseMap(mapKafkaSaslMechanismFromGRPC).(map[chmodels.KafkaSaslMechanism]config.ClickhouseConfig_Kafka_SaslMechanism)
)

func KafkaSaslMechanismToGRPC(mechanism chmodels.KafkaSaslMechanism) config.ClickhouseConfig_Kafka_SaslMechanism {
	v, ok := mapKafkaSaslMechanismToGRPC[mechanism]
	if !ok {
		return config.ClickhouseConfig_Kafka_SASL_MECHANISM_UNSPECIFIED
	}

	return v
}

func KafkaSaslMechanismFromGRPC(mechanism config.ClickhouseConfig_Kafka_SaslMechanism) (chmodels.KafkaSaslMechanism, error) {
	v, ok := mapKafkaSaslMechanismFromGRPC[mechanism]
	if !ok {
		return chmodels.KafkaSaslMechanismUnspecified, semerr.InvalidInputf("unknown kafka sasl mechanism %q", mechanism.String())
	}

	return v, nil
}

func KafkaFromGRPC(k *config.ClickhouseConfig_Kafka) (chmodels.Kafka, error) {
	if k == nil {
		return chmodels.Kafka{}, nil
	}

	sp, err := KafkaSecurityProtocolFromGRPC(k.SecurityProtocol)
	if err != nil {
		return chmodels.Kafka{}, err
	}

	sm, err := KafkaSaslMechanismFromGRPC(k.SaslMechanism)
	if err != nil {
		return chmodels.Kafka{}, err
	}
	return chmodels.Kafka{
		SecurityProtocol: sp,
		SaslMechanism:    sm,
		SaslUsername:     grpc.OptionalStringFromGRPC(k.SaslUsername),
		SaslPassword:     grpc.OptionalPasswordFromGRPC(k.SaslPassword),

		Valid: true,
	}, nil
}

func KafkaTopicFromGRPC(t *config.ClickhouseConfig_KafkaTopic) (chmodels.KafkaTopic, error) {
	settings, err := KafkaFromGRPC(t.GetSettings())
	if err != nil {
		return chmodels.KafkaTopic{}, err
	}

	return chmodels.KafkaTopic{
		Name:     t.GetName(),
		Settings: settings,
	}, nil
}

func KafkaTopicsFromGRPC(ts []*config.ClickhouseConfig_KafkaTopic) ([]chmodels.KafkaTopic, error) {
	res := make([]chmodels.KafkaTopic, 0, len(ts))
	for _, t := range ts {
		topic, err := KafkaTopicFromGRPC(t)
		if err != nil {
			return nil, err
		}
		res = append(res, topic)
	}

	return res, nil
}

func KafkaToGRPC(kf chmodels.Kafka) *config.ClickhouseConfig_Kafka {
	if !kf.Valid {
		return &config.ClickhouseConfig_Kafka{}
	}

	return &config.ClickhouseConfig_Kafka{
		SecurityProtocol: KafkaSecurityProtocolToGRPC(kf.SecurityProtocol),
		SaslMechanism:    KafkaSaslMechanismToGRPC(kf.SaslMechanism),
		SaslUsername:     kf.SaslUsername.String,
	}
}

func KafkaTopicToGRPC(kft chmodels.KafkaTopic) *config.ClickhouseConfig_KafkaTopic {
	return &config.ClickhouseConfig_KafkaTopic{
		Name:     kft.Name,
		Settings: KafkaToGRPC(kft.Settings),
	}
}

func KafkaTopicsToGRPC(kfts []chmodels.KafkaTopic) []*config.ClickhouseConfig_KafkaTopic {
	res := make([]*config.ClickhouseConfig_KafkaTopic, len(kfts))
	for i, topic := range kfts {
		res[i] = KafkaTopicToGRPC(topic)
	}
	return res
}

// Graphite rollup configs mapping

func GraphiteRollupToGRPC(gr chmodels.GraphiteRollup) *config.ClickhouseConfig_GraphiteRollup {
	return &config.ClickhouseConfig_GraphiteRollup{
		Name:     gr.Name,
		Patterns: GraphiteRollupPatternsToGRPC(gr.Patterns),
	}
}

func GraphiteRollupsToGRPC(grs []chmodels.GraphiteRollup) []*config.ClickhouseConfig_GraphiteRollup {
	res := make([]*config.ClickhouseConfig_GraphiteRollup, len(grs))
	for i, rollup := range grs {
		res[i] = GraphiteRollupToGRPC(rollup)
	}
	return res
}

func GraphiteRollupPatternToGRPC(grp chmodels.GraphiteRollupPattern) *config.ClickhouseConfig_GraphiteRollup_Pattern {
	return &config.ClickhouseConfig_GraphiteRollup_Pattern{
		Regexp:    grp.Regexp.String,
		Function:  grp.Function,
		Retention: GraphiteRollupPatternRetentionsToGRPC(grp.Retentionts),
	}
}

func GraphiteRollupPatternsToGRPC(grps []chmodels.GraphiteRollupPattern) []*config.ClickhouseConfig_GraphiteRollup_Pattern {
	res := make([]*config.ClickhouseConfig_GraphiteRollup_Pattern, len(grps))
	for i, pattern := range grps {
		res[i] = GraphiteRollupPatternToGRPC(pattern)
	}
	return res
}

func GraphiteRollupPatternRetentionToGRPC(grpr chmodels.GraphiteRollupPatternRetention) *config.ClickhouseConfig_GraphiteRollup_Pattern_Retention {
	return &config.ClickhouseConfig_GraphiteRollup_Pattern_Retention{
		Age:       grpr.Age,
		Precision: grpr.Precision,
	}
}

func GraphiteRollupPatternRetentionsToGRPC(grprs []chmodels.GraphiteRollupPatternRetention) []*config.ClickhouseConfig_GraphiteRollup_Pattern_Retention {
	res := make([]*config.ClickhouseConfig_GraphiteRollup_Pattern_Retention, len(grprs))
	for i, retention := range grprs {
		res[i] = GraphiteRollupPatternRetentionToGRPC(retention)
	}
	return res
}

func GraphiteRollupFromGRPC(gr *config.ClickhouseConfig_GraphiteRollup) chmodels.GraphiteRollup {
	patterns := make([]chmodels.GraphiteRollupPattern, 0, len(gr.Patterns))
	for _, p := range gr.GetPatterns() {
		retentions := make([]chmodels.GraphiteRollupPatternRetention, 0, len(p.Retention))
		for _, r := range p.GetRetention() {
			retentions = append(retentions, chmodels.GraphiteRollupPatternRetention{
				Age:       r.GetAge(),
				Precision: r.GetPrecision(),
			})
		}

		patterns = append(patterns, chmodels.GraphiteRollupPattern{
			Regexp:      grpc.OptionalStringFromGRPC(p.GetRegexp()),
			Function:    p.GetFunction(),
			Retentionts: retentions,
		})
	}

	return chmodels.GraphiteRollup{
		Name:     gr.GetName(),
		Patterns: patterns,
	}
}

func GraphiteRollupsFromGRPC(grs []*config.ClickhouseConfig_GraphiteRollup) []chmodels.GraphiteRollup {
	res := make([]chmodels.GraphiteRollup, 0, len(grs))
	for _, gr := range grs {
		res = append(res, GraphiteRollupFromGRPC(gr))
	}

	return res
}

// RabbitMQ mapping

func RabbitMQFromGRPC(r *config.ClickhouseConfig_Rabbitmq) chmodels.RabbitMQ {
	return chmodels.RabbitMQ{
		Username: grpc.OptionalStringFromGRPC(r.GetUsername()),
		Password: grpc.OptionalPasswordFromGRPC(r.GetPassword()),

		Valid: true,
	}
}

func RabbitMQToGRPC(rmq chmodels.RabbitMQ) *config.ClickhouseConfig_Rabbitmq {
	if !rmq.Valid {
		return &config.ClickhouseConfig_Rabbitmq{}
	}

	return &config.ClickhouseConfig_Rabbitmq{
		Username: rmq.Username.String,
	}
}

// Dictionaries config mapping

func DictionaryFromGRPC(cfg *config.ClickhouseConfig_ExternalDictionary) (chmodels.Dictionary, error) {
	dict := chmodels.Dictionary{
		Name:      cfg.GetName(),
		Structure: DictionaryStructureFromGRPC(cfg.GetStructure()),
	}

	layout, err := DictionaryLayoutFromGRPC(cfg.GetLayout())
	if err != nil {
		return chmodels.Dictionary{}, err
	}

	dict.Layout = layout

	switch lifetime := cfg.GetLifetime().(type) {
	case *config.ClickhouseConfig_ExternalDictionary_FixedLifetime:
		dict.FixedLifetime = optional.NewInt64(lifetime.FixedLifetime)
	case *config.ClickhouseConfig_ExternalDictionary_LifetimeRange:
		dict.LifetimeRange = chmodels.DictionaryLifetimeRange{
			Min:   lifetime.LifetimeRange.Min,
			Max:   lifetime.LifetimeRange.Max,
			Valid: true,
		}
	}

	switch source := cfg.GetSource().(type) {
	case *config.ClickhouseConfig_ExternalDictionary_HttpSource_:
		dict.HTTPSource = DictionarySourceHTTPFromGRPC(source)
	case *config.ClickhouseConfig_ExternalDictionary_MysqlSource_:
		dict.MySQLSource = DictionarySourceMySQLFromGRPC(source)
	case *config.ClickhouseConfig_ExternalDictionary_ClickhouseSource_:
		dict.ClickhouseSource = DictionarySourceClickhouseFromGRPC(source)
	case *config.ClickhouseConfig_ExternalDictionary_MongodbSource_:
		dict.MongoDBSource = DictionarySourceMongoDBFromGRPC(source)
	case *config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_:
		dict.PostgreSQLSource, err = DictionarySourcePostgreSQLFromGRPC(source)
		if err != nil {
			return chmodels.Dictionary{}, err
		}
	case *config.ClickhouseConfig_ExternalDictionary_YtSource_:
		dict.YTSource, err = DictionarySourceYTFromGRPC(source)
		if err != nil {
			return chmodels.Dictionary{}, err
		}
	}

	return dict, nil
}

func DictionariesFromGRPC(dicts []*config.ClickhouseConfig_ExternalDictionary) ([]chmodels.Dictionary, error) {
	res := make([]chmodels.Dictionary, 0, len(dicts))
	for _, c := range dicts {
		c, err := DictionaryFromGRPC(c)
		if err != nil {
			return nil, err
		}
		res = append(res, c)
	}

	return res, nil
}

func DictionaryToGRPC(d chmodels.Dictionary) *config.ClickhouseConfig_ExternalDictionary {
	res := &config.ClickhouseConfig_ExternalDictionary{
		Name:      d.Name,
		Structure: DictionaryStructureToGRPC(d.Structure),
		Layout:    DictionaryLayoutToGRPC(d.Layout),
	}

	switch {
	case d.FixedLifetime.Valid:
		res.Lifetime = &config.ClickhouseConfig_ExternalDictionary_FixedLifetime{
			FixedLifetime: d.FixedLifetime.Must(),
		}
	case d.LifetimeRange.Valid:
		res.Lifetime = &config.ClickhouseConfig_ExternalDictionary_LifetimeRange{
			LifetimeRange: DictionaryLifetimeRangeToGRPC(d.LifetimeRange),
		}
	}

	switch {
	case d.HTTPSource.Valid:
		res.Source = DictionarySourceHTTPToGRPC(d.HTTPSource)
	case d.MySQLSource.Valid:
		res.Source = DictionarySourceMySQLToGRPC(d.MySQLSource)
	case d.ClickhouseSource.Valid:
		res.Source = DictionarySourceClickhouseToGRPC(d.ClickhouseSource)
	case d.MongoDBSource.Valid:
		res.Source = DictionarySourceMongoDBToGRPC(d.MongoDBSource)
	case d.PostgreSQLSource.Valid:
		res.Source = DictionarySourcePostgreSQLToGRPC(d.PostgreSQLSource)
	case d.YTSource.Valid:
		res.Source = DictionarySourceYTToGRPC(d.YTSource)
	}

	return res
}

func DictionariesToGRPC(d []chmodels.Dictionary) []*config.ClickhouseConfig_ExternalDictionary {
	res := make([]*config.ClickhouseConfig_ExternalDictionary, len(d))
	for i, dictionary := range d {
		res[i] = DictionaryToGRPC(dictionary)
	}
	return res
}

func DictionaryLifetimeRangeToGRPC(dr chmodels.DictionaryLifetimeRange) *config.ClickhouseConfig_ExternalDictionary_Range {
	if !dr.Valid {
		return nil
	}

	return &config.ClickhouseConfig_ExternalDictionary_Range{
		Min: dr.Min,
		Max: dr.Max,
	}
}

// Dictionary structure mapping

func DictionaryStructureToGRPC(ds chmodels.DictionaryStructure) *config.ClickhouseConfig_ExternalDictionary_Structure {
	return &config.ClickhouseConfig_ExternalDictionary_Structure{
		Id:         DictionaryStructureIDToGRPC(ds.ID),
		Key:        DictionaryStructureKeyToGRPC(ds.Key),
		RangeMin:   DictionaryStructureAttributeToGRPC(ds.RangeMin),
		RangeMax:   DictionaryStructureAttributeToGRPC(ds.RangeMax),
		Attributes: DictionaryStructureAttributesToGRPC(ds.Attributes),
	}
}

func DictionaryStructureFromGRPC(structure *config.ClickhouseConfig_ExternalDictionary_Structure) chmodels.DictionaryStructure {
	res := chmodels.DictionaryStructure{
		RangeMin:   DictionaryStructureAttributeFromGRPC(structure.GetRangeMin()),
		RangeMax:   DictionaryStructureAttributeFromGRPC(structure.GetRangeMax()),
		Attributes: DictionaryStructureAttributesFromGRPC(structure.GetAttributes()),
	}

	if id := structure.GetId(); id != nil {
		res.ID = optional.NewString(id.Name)
	}

	if key := structure.GetKey(); key != nil {
		res.Key = chmodels.DictionaryStructureKey{
			Attributes: DictionaryStructureAttributesFromGRPC(key.GetAttributes()),
			Valid:      true,
		}
	}

	return res
}

func DictionaryStructureAttributeFromGRPC(attr *config.ClickhouseConfig_ExternalDictionary_Structure_Attribute) chmodels.DictionaryStructureAttribute {
	if attr == nil {
		return chmodels.DictionaryStructureAttribute{}
	}

	return chmodels.DictionaryStructureAttribute{
		Name:         attr.GetName(),
		Type:         attr.GetType(),
		NullValue:    optional.NewString(attr.GetNullValue()),
		Expression:   grpc.OptionalStringFromGRPC(attr.GetExpression()),
		Hierarchical: optional.NewBool(attr.GetHierarchical()),
		Injective:    optional.NewBool(attr.GetInjective()),
		Valid:        true,
	}
}

func DictionaryStructureAttributesFromGRPC(attrs []*config.ClickhouseConfig_ExternalDictionary_Structure_Attribute) []chmodels.DictionaryStructureAttribute {
	res := make([]chmodels.DictionaryStructureAttribute, 0, len(attrs))
	for _, c := range attrs {
		res = append(res, DictionaryStructureAttributeFromGRPC(c))
	}

	return res
}

func DictionaryStructureIDToGRPC(dsid optional.String) *config.ClickhouseConfig_ExternalDictionary_Structure_Id {
	if !dsid.Valid {
		return nil
	}

	return &config.ClickhouseConfig_ExternalDictionary_Structure_Id{
		Name: dsid.String,
	}
}

func DictionaryStructureKeyToGRPC(dsk chmodels.DictionaryStructureKey) *config.ClickhouseConfig_ExternalDictionary_Structure_Key {
	if !dsk.Valid {
		return nil
	}

	return &config.ClickhouseConfig_ExternalDictionary_Structure_Key{
		Attributes: DictionaryStructureAttributesToGRPC(dsk.Attributes),
	}
}

func DictionaryStructureAttributeToGRPC(dsa chmodels.DictionaryStructureAttribute) *config.ClickhouseConfig_ExternalDictionary_Structure_Attribute {
	if !dsa.Valid {
		return nil
	}

	return &config.ClickhouseConfig_ExternalDictionary_Structure_Attribute{
		Name:         dsa.Name,
		Type:         dsa.Type,
		NullValue:    dsa.NullValue.String,
		Expression:   dsa.Expression.String,
		Hierarchical: dsa.Hierarchical.Bool,
		Injective:    dsa.Hierarchical.Bool,
	}
}

func DictionaryStructureAttributesToGRPC(dsas []chmodels.DictionaryStructureAttribute) []*config.ClickhouseConfig_ExternalDictionary_Structure_Attribute {
	res := make([]*config.ClickhouseConfig_ExternalDictionary_Structure_Attribute, len(dsas))
	for i, attribute := range dsas {
		res[i] = DictionaryStructureAttributeToGRPC(attribute)
	}
	return res
}

// Dictionary Layout mapping

var (
	mapDictionaryLayoutTypeFromGRPC = map[config.ClickhouseConfig_ExternalDictionary_Layout_Type]chmodels.DictionaryLayoutType{
		config.ClickhouseConfig_ExternalDictionary_Layout_FLAT:               chmodels.DictionaryLayoutTypeFlat,
		config.ClickhouseConfig_ExternalDictionary_Layout_HASHED:             chmodels.DictionaryLayoutTypeHashed,
		config.ClickhouseConfig_ExternalDictionary_Layout_COMPLEX_KEY_HASHED: chmodels.DictionaryLayoutTypeComplexKeyHashed,
		config.ClickhouseConfig_ExternalDictionary_Layout_RANGE_HASHED:       chmodels.DictionaryLayoutTypeRangeHashed,
		config.ClickhouseConfig_ExternalDictionary_Layout_CACHE:              chmodels.DictionaryLayoutTypeCache,
		config.ClickhouseConfig_ExternalDictionary_Layout_COMPLEX_KEY_CACHE:  chmodels.DictionaryLayoutTypeComplexKeyCache,
	}
	mapDictionaryLayoutTypeToGRPC = reflectutil.ReverseMap(mapDictionaryLayoutTypeFromGRPC).(map[chmodels.DictionaryLayoutType]config.ClickhouseConfig_ExternalDictionary_Layout_Type)
)

func DictionaryLayoutTypeToGRPC(l chmodels.DictionaryLayoutType) config.ClickhouseConfig_ExternalDictionary_Layout_Type {
	v, ok := mapDictionaryLayoutTypeToGRPC[l]
	if !ok {
		return config.ClickhouseConfig_ExternalDictionary_Layout_TYPE_UNSPECIFIED
	}

	return v
}

func DictionaryLayoutTypeFromGRPC(l config.ClickhouseConfig_ExternalDictionary_Layout_Type) (chmodels.DictionaryLayoutType, error) {
	v, ok := mapDictionaryLayoutTypeFromGRPC[l]
	if !ok {
		return chmodels.DictionaryLayoutTypeFlat, semerr.InvalidInputf("unknown dictionary layout type %q", l.String())
	}

	return v, nil
}

func DictionaryLayoutFromGRPC(layout *config.ClickhouseConfig_ExternalDictionary_Layout) (chmodels.DictionaryLayout, error) {
	t, err := DictionaryLayoutTypeFromGRPC(layout.GetType())
	if err != nil {
		return chmodels.DictionaryLayout{}, err
	}

	if layout.GetSizeInCells() == 0 {
		return chmodels.DictionaryLayout{Type: t}, nil
	}

	return chmodels.DictionaryLayout{
		Type:        t,
		SizeInCells: optional.NewInt64(layout.SizeInCells),
	}, nil
}

func DictionaryLayoutToGRPC(dl chmodels.DictionaryLayout) *config.ClickhouseConfig_ExternalDictionary_Layout {
	return &config.ClickhouseConfig_ExternalDictionary_Layout{
		Type:        DictionaryLayoutTypeToGRPC(dl.Type),
		SizeInCells: dl.SizeInCells.Int64,
	}
}

// Dictionary sources configs mapping

func DictionarySourceHTTPToGRPC(dsh chmodels.DictionarySourceHTTP) *config.ClickhouseConfig_ExternalDictionary_HttpSource_ {
	return &config.ClickhouseConfig_ExternalDictionary_HttpSource_{
		HttpSource: &config.ClickhouseConfig_ExternalDictionary_HttpSource{
			Url:    dsh.URL,
			Format: dsh.Format,
		},
	}
}

func DictionarySourceHTTPFromGRPC(source *config.ClickhouseConfig_ExternalDictionary_HttpSource_) chmodels.DictionarySourceHTTP {
	return chmodels.DictionarySourceHTTP{
		URL:    source.HttpSource.GetUrl(),
		Format: source.HttpSource.GetFormat(),
		Valid:  true,
	}
}

func DictionarySourceMySQLToGRPC(dsms chmodels.DictionarySourceMySQL) *config.ClickhouseConfig_ExternalDictionary_MysqlSource_ {
	return &config.ClickhouseConfig_ExternalDictionary_MysqlSource_{
		MysqlSource: &config.ClickhouseConfig_ExternalDictionary_MysqlSource{
			Db:              dsms.DB,
			Table:           dsms.Table,
			Port:            dsms.Port.Int64,
			User:            dsms.User.String,
			Replicas:        DictionarySourceMySQLReplicasToGRPC(dsms.Replicas),
			Where:           dsms.Where.String,
			InvalidateQuery: dsms.InvalidateQuery.String,
		},
	}
}

func DictionarySourceMySQLReplicaToGRPC(dsmsr chmodels.DictionarySourceMySQLReplica) *config.ClickhouseConfig_ExternalDictionary_MysqlSource_Replica {
	return &config.ClickhouseConfig_ExternalDictionary_MysqlSource_Replica{
		Host:     dsmsr.Host,
		Priority: dsmsr.Priority.Int64,
		Port:     dsmsr.Port.Int64,
		User:     dsmsr.User.String,
	}
}

func DictionarySourceMySQLReplicasToGRPC(dsmsrs []chmodels.DictionarySourceMySQLReplica) []*config.ClickhouseConfig_ExternalDictionary_MysqlSource_Replica {
	res := make([]*config.ClickhouseConfig_ExternalDictionary_MysqlSource_Replica, len(dsmsrs))
	for i, replica := range dsmsrs {
		res[i] = DictionarySourceMySQLReplicaToGRPC(replica)
	}
	return res
}

func DictionarySourceMySQLFromGRPC(source *config.ClickhouseConfig_ExternalDictionary_MysqlSource_) chmodels.DictionarySourceMySQL {
	replicas := make([]chmodels.DictionarySourceMySQLReplica, 0, len(source.MysqlSource.Replicas))
	for _, replica := range source.MysqlSource.Replicas {
		priority := chmodels.DefaultMySQLReplicaPriority
		if replica.GetPriority() != 0 {
			priority = replica.GetPriority()
		}

		replicas = append(replicas, chmodels.DictionarySourceMySQLReplica{
			Host:     replica.GetHost(),
			Priority: optional.NewInt64(priority),
			Port:     grpc.OptionalInt64ZeroInvalidFromGRPC(replica.GetPort()),
			User:     grpc.OptionalStringFromGRPC(replica.GetUser()),
			Password: grpc.OptionalPasswordFromGRPC(replica.Password),
		})
	}

	return chmodels.DictionarySourceMySQL{
		DB:              source.MysqlSource.GetDb(),
		Table:           source.MysqlSource.GetTable(),
		Port:            grpc.OptionalInt64ZeroInvalidFromGRPC(source.MysqlSource.GetPort()),
		User:            grpc.OptionalStringFromGRPC(source.MysqlSource.GetUser()),
		Password:        grpc.OptionalPasswordFromGRPC(source.MysqlSource.Password),
		Replicas:        replicas,
		Where:           grpc.OptionalStringFromGRPC(source.MysqlSource.GetWhere()),
		InvalidateQuery: grpc.OptionalStringFromGRPC(source.MysqlSource.GetInvalidateQuery()),
		Valid:           true,
	}
}

func DictionarySourceClickhouseToGRPC(dsch chmodels.DictionarySourceClickhouse) *config.ClickhouseConfig_ExternalDictionary_ClickhouseSource_ {
	return &config.ClickhouseConfig_ExternalDictionary_ClickhouseSource_{
		ClickhouseSource: &config.ClickhouseConfig_ExternalDictionary_ClickhouseSource{
			Db:    dsch.DB,
			Table: dsch.Table,
			Host:  dsch.Host,
			Port:  dsch.Port.Int64,
			User:  dsch.User,
			Where: dsch.Where.String,
		},
	}
}

func DictionarySourceClickhouseFromGRPC(source *config.ClickhouseConfig_ExternalDictionary_ClickhouseSource_) chmodels.DictionarySourceClickhouse {
	port := chmodels.DefaultClickHousePort
	if source.ClickhouseSource.Port != 0 {
		port = source.ClickhouseSource.Port
	}

	return chmodels.DictionarySourceClickhouse{
		DB:       source.ClickhouseSource.GetDb(),
		Table:    source.ClickhouseSource.GetTable(),
		Host:     source.ClickhouseSource.GetHost(),
		Port:     optional.NewInt64(port),
		User:     source.ClickhouseSource.GetUser(),
		Password: grpc.OptionalPasswordFromGRPC(source.ClickhouseSource.GetPassword()),
		Where:    grpc.OptionalStringFromGRPC(source.ClickhouseSource.GetWhere()),
		Valid:    true,
	}
}

func DictionarySourceMongoDBToGRPC(dsmd chmodels.DictionarySourceMongoDB) *config.ClickhouseConfig_ExternalDictionary_MongodbSource_ {
	return &config.ClickhouseConfig_ExternalDictionary_MongodbSource_{
		MongodbSource: &config.ClickhouseConfig_ExternalDictionary_MongodbSource{
			Db:         dsmd.DB,
			Collection: dsmd.Collection,
			Host:       dsmd.Host,
			Port:       dsmd.Port.Int64,
			User:       dsmd.User,
		},
	}
}

func DictionarySourceMongoDBFromGRPC(source *config.ClickhouseConfig_ExternalDictionary_MongodbSource_) chmodels.DictionarySourceMongoDB {
	port := chmodels.DefaultMongoDBPort
	if source.MongodbSource.GetPort() != 0 {
		port = source.MongodbSource.GetPort()
	}

	return chmodels.DictionarySourceMongoDB{
		DB:         source.MongodbSource.GetDb(),
		Collection: source.MongodbSource.GetCollection(),
		Host:       source.MongodbSource.GetHost(),
		Port:       optional.NewInt64(port),
		User:       source.MongodbSource.GetUser(),
		Password:   grpc.OptionalPasswordFromGRPC(source.MongodbSource.GetPassword()),
		Valid:      true,
	}
}

var (
	mapPostgresSSLModeFromGRPC = map[config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_SslMode]chmodels.PostgresSSLMode{
		config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_SSL_MODE_UNSPECIFIED: chmodels.PostgresSSLModeUnspecified,
		config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_DISABLE:              chmodels.PostgresSSLModeDisable,
		config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_ALLOW:                chmodels.PostgresSSLModeAllow,
		config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_PREFER:               chmodels.PostgresSSLModePrefer,
		config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_VERIFY_CA:            chmodels.PostgresSSLModeVerifyCA,
		config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_VERIFY_FULL:          chmodels.PostgresSSLModeVerifyFull,
	}
	mapPostgresSSLModeToGRPC = reflectutil.ReverseMap(mapPostgresSSLModeFromGRPC).(map[chmodels.PostgresSSLMode]config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_SslMode)
)

func PostgresSSLModeToGRPC(l chmodels.PostgresSSLMode) config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_SslMode {
	v, ok := mapPostgresSSLModeToGRPC[l]
	if !ok {
		return config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_SSL_MODE_UNSPECIFIED
	}

	return v
}

func PostgresSSLModeFromGRPC(l config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_SslMode) (chmodels.PostgresSSLMode, error) {
	v, ok := mapPostgresSSLModeFromGRPC[l]
	if !ok {
		return chmodels.PostgresSSLModeUnspecified, semerr.InvalidInputf("unknown postgresql ssl mode %q", l.String())
	}

	return v, nil
}

func DictionarySourcePostgreSQLToGRPC(dspg chmodels.DictionarySourcePostgreSQL) *config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_ {
	return &config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_{
		PostgresqlSource: &config.ClickhouseConfig_ExternalDictionary_PostgresqlSource{
			Db:              dspg.DB,
			Table:           dspg.Table,
			Hosts:           dspg.Hosts,
			Port:            dspg.Port.Int64,
			User:            dspg.User,
			InvalidateQuery: dspg.InvalidateQuery.String,
			SslMode:         PostgresSSLModeToGRPC(dspg.SSLMode),
		},
	}
}

func DictionarySourcePostgreSQLFromGRPC(source *config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_) (chmodels.DictionarySourcePostgreSQL, error) {
	mode, err := PostgresSSLModeFromGRPC(source.PostgresqlSource.SslMode)
	if err != nil {
		return chmodels.DictionarySourcePostgreSQL{}, err
	}

	port := chmodels.DefaultPostgreSQLPort
	if source.PostgresqlSource.GetPort() != 0 {
		port = source.PostgresqlSource.GetPort()
	}

	return chmodels.DictionarySourcePostgreSQL{
		DB:              source.PostgresqlSource.GetDb(),
		Table:           source.PostgresqlSource.GetTable(),
		Hosts:           source.PostgresqlSource.GetHosts(),
		Port:            optional.NewInt64(port),
		User:            source.PostgresqlSource.GetUser(),
		Password:        grpc.OptionalPasswordFromGRPC(source.PostgresqlSource.GetPassword()),
		InvalidateQuery: grpc.OptionalStringFromGRPC(source.PostgresqlSource.GetInvalidateQuery()),
		SSLMode:         mode,
		Valid:           true,
	}, nil
}

var (
	mapYTClusterSelectionFromGRPC = map[config.ClickhouseConfig_ExternalDictionary_YtSource_ClusterSelection]chmodels.YTClusterSelection{
		config.ClickhouseConfig_ExternalDictionary_YtSource_CLUSTER_SELECTION_UNSPECIFIED: chmodels.YTClusterSelectionUnspecified,
		config.ClickhouseConfig_ExternalDictionary_YtSource_ORDERED:                       chmodels.YTClusterSelectionOrdered,
		config.ClickhouseConfig_ExternalDictionary_YtSource_RANDOM:                        chmodels.YTClusterSelectionRandom,
	}
	mapYTClusterSelectionToGRPC = reflectutil.ReverseMap(mapYTClusterSelectionFromGRPC).(map[chmodels.YTClusterSelection]config.ClickhouseConfig_ExternalDictionary_YtSource_ClusterSelection)
)

func YTClusterSelectionToGRPC(cs chmodels.YTClusterSelection) config.ClickhouseConfig_ExternalDictionary_YtSource_ClusterSelection {
	result, ok := mapYTClusterSelectionToGRPC[cs]
	if !ok {
		return config.ClickhouseConfig_ExternalDictionary_YtSource_CLUSTER_SELECTION_UNSPECIFIED
	}

	return result
}

func YTClusterSelectionFromGRPC(cs config.ClickhouseConfig_ExternalDictionary_YtSource_ClusterSelection) (chmodels.YTClusterSelection, error) {
	result, ok := mapYTClusterSelectionFromGRPC[cs]
	if !ok {
		return chmodels.YTClusterSelectionUnspecified, semerr.InvalidInputf("unknown YT cluster selection %q", cs.String())
	}

	return result, nil
}

func DictionarySourceYTToGRPC(source chmodels.DictionarySourceYT) *config.ClickhouseConfig_ExternalDictionary_YtSource_ {
	return &config.ClickhouseConfig_ExternalDictionary_YtSource_{
		YtSource: &config.ClickhouseConfig_ExternalDictionary_YtSource{
			Clusters:            source.Clusters,
			Table:               source.Table,
			Keys:                source.Keys,
			Fields:              source.Fields,
			DateFields:          source.DateFields,
			DatetimeFields:      source.DatetimeFields,
			Query:               source.Query.String,
			User:                source.User.String,
			ClusterSelection:    YTClusterSelectionToGRPC(source.ClusterSelection),
			UseQueryForCache:    grpc.OptionalBoolToGRPC(source.UseQueryForCache),
			ForceReadTable:      grpc.OptionalBoolToGRPC(source.ForceReadTable),
			RangeExpansionLimit: grpc.OptionalInt64ToGRPC(source.RangeExpansionLimit),
			InputRowLimit:       grpc.OptionalInt64ToGRPC(source.InputRowLimit),
			OutputRowLimit:      grpc.OptionalInt64ToGRPC(source.OutputRowLimit),
			YtSocketTimeout:     grpc.OptionalInt64ToGRPC(source.YTSocketTimeout),
			YtConnectionTimeout: grpc.OptionalInt64ToGRPC(source.YTConnectionTimeout),
			YtLookupTimeout:     grpc.OptionalInt64ToGRPC(source.YTLookupTimeout),
			YtSelectTimeout:     grpc.OptionalInt64ToGRPC(source.YTSelectTimeout),
			YtRetryCount:        grpc.OptionalInt64ToGRPC(source.YTRetryCount),
		},
	}
}

func DictionarySourceYTFromGRPC(source *config.ClickhouseConfig_ExternalDictionary_YtSource_) (chmodels.DictionarySourceYT, error) {
	clusterSelection, err := YTClusterSelectionFromGRPC(source.YtSource.ClusterSelection)
	if err != nil {
		return chmodels.DictionarySourceYT{}, err
	}

	yt := chmodels.DictionarySourceYT{
		Clusters:            source.YtSource.GetClusters(),
		Table:               source.YtSource.GetTable(),
		Keys:                source.YtSource.GetKeys(),
		Fields:              source.YtSource.GetFields(),
		DateFields:          source.YtSource.GetDateFields(),
		DatetimeFields:      source.YtSource.GetDatetimeFields(),
		Query:               grpc.OptionalStringFromGRPC(source.YtSource.GetQuery()),
		User:                grpc.OptionalStringFromGRPC(source.YtSource.GetUser()),
		Token:               grpc.OptionalPasswordFromGRPC(source.YtSource.GetToken()),
		ClusterSelection:    clusterSelection,
		UseQueryForCache:    grpc.OptionalBoolFromGRPC(source.YtSource.GetUseQueryForCache()),
		ForceReadTable:      grpc.OptionalBoolFromGRPC(source.YtSource.GetForceReadTable()),
		RangeExpansionLimit: grpc.OptionalInt64FromGRPC(source.YtSource.GetRangeExpansionLimit()),
		InputRowLimit:       grpc.OptionalInt64FromGRPC(source.YtSource.GetInputRowLimit()),
		OutputRowLimit:      grpc.OptionalInt64FromGRPC(source.YtSource.GetOutputRowLimit()),
		YTSocketTimeout:     grpc.OptionalInt64FromGRPC(source.YtSource.GetYtSocketTimeout()),
		YTConnectionTimeout: grpc.OptionalInt64FromGRPC(source.YtSource.GetYtConnectionTimeout()),
		YTLookupTimeout:     grpc.OptionalInt64FromGRPC(source.YtSource.GetYtLookupTimeout()),
		YTSelectTimeout:     grpc.OptionalInt64FromGRPC(source.YtSource.GetYtSelectTimeout()),
		YTRetryCount:        grpc.OptionalInt64FromGRPC(source.YtSource.GetYtRetryCount()),
		Valid:               true,
	}
	return yt, nil
}
