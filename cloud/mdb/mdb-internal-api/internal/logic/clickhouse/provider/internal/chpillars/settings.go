package chpillars

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
)

type ClickHouseConfig struct {
	MergeTree       MergeTree        `json:"merge_tree,omitempty" mapping:"map"`
	Compressions    []Compression    `json:"compression,omitempty" mapping:"omit"`
	Dictionaries    []Dictionary     `json:"dictionaries,omitempty" mapping:"omit"`
	GraphiteRollups []GraphiteRollup `json:"graphite_rollup,omitempty" mapping:"omit"`
	Kafka           *Kafka           `json:"kafka,omitempty" mapping:"omit"`
	KafkaTopics     []KafkaTopic     `json:"kafka_topics,omitempty" mapping:"omit"`
	RabbitMQ        *RabbitMQ        `json:"rabbitmq,omitempty" mapping:"omit"`

	BackgroundPoolSize                    *int64 `name:"background_pool_size" json:"background_pool_size,omitempty"`
	BackgroundFetchesPoolSize             *int64 `name:"background_fetches_pool_size" json:"background_fetches_pool_size,omitempty"`
	BackgroundSchedulePoolSize            *int64 `name:"background_schedule_pool_size" json:"background_schedule_pool_size,omitempty"`
	BackgroundMovePoolSize                *int64 `name:"background_move_pool_size" json:"background_move_pool_size,omitempty"`
	BackgroundDistributedSchedulePoolSize *int64 `name:"background_distributed_schedule_pool_size" json:"background_distributed_schedule_pool_size,omitempty"`
	BackgroundBufferFlushSchedulePoolSize *int64 `name:"background_buffer_flush_schedule_pool_size" json:"background_buffer_flush_schedule_pool_size,omitempty"`

	LogLevel                    *string `name:"log_level" json:"log_level,omitempty"`
	QueryLogRetentionSize       *int64  `name:"query_log_retention_size" json:"query_log_retention_size,omitempty"`
	QueryLogRetentionTime       *int64  `name:"query_log_retention_time" json:"query_log_retention_time,omitempty"`
	QueryThreadLogEnabled       *bool   `name:"query_thread_log_enabled" json:"query_thread_log_enabled,omitempty"`
	QueryThreadLogRetentionSize *int64  `name:"query_thread_log_retention_size" json:"query_thread_log_retention_size,omitempty"`
	QueryThreadLogRetentionTime *int64  `name:"query_thread_log_retention_time" json:"query_thread_log_retention_time,omitempty"`
	PartLogRetentionSize        *int64  `name:"part_log_retention_size" json:"part_log_retention_size,omitempty"`
	PartLogRetentionTime        *int64  `name:"part_log_retention_time" json:"part_log_retention_time,omitempty"`
	MetricLogEnabled            *bool   `name:"metric_log_enabled" json:"metric_log_enabled,omitempty"`
	MetricLogRetentionSize      *int64  `name:"metric_log_retention_size" json:"metric_log_retention_size,omitempty"`
	MetricLogRetentionTime      *int64  `name:"metric_log_retention_time" json:"metric_log_retention_time,omitempty"`
	TraceLogEnabled             *bool   `name:"trace_log_enabled" json:"trace_log_enabled,omitempty"`
	TraceLogRetentionSize       *int64  `name:"trace_log_retention_size" json:"trace_log_retention_size,omitempty"`
	TraceLogRetentionTime       *int64  `name:"trace_log_retention_time" json:"trace_log_retention_time,omitempty"`
	TextLogEnabled              *bool   `name:"text_log_enabled" json:"text_log_enabled,omitempty"`
	TextLogRetentionSize        *int64  `name:"text_log_retention_size" json:"text_log_retention_size,omitempty"`
	TextLogRetentionTime        *int64  `name:"text_log_retention_time" json:"text_log_retention_time,omitempty"`
	TextLogLevel                *string `name:"text_log_level" json:"text_log_level,omitempty"`
	OpenTelemetrySpanLogEnabled *bool   `name:"opentelemetry_span_log_enabled" json:"opentelemetry_span_log_enabled,omitempty"`

	BuiltinDictionariesReloadInterval *int64  `name:"builtin_dictionaries_reload_interval" json:"builtin_dictionaries_reload_interval,omitempty"`
	DefaultDatabase                   *string `name:"default_database" json:"default_database,omitempty"`
	DefaultProfile                    *string `name:"default_profile" json:"default_profile,omitempty"`
	GeobaseURI                        *string `name:"geobase_uri" json:"geobase_uri,omitempty"`
	KeepAliveTimeout                  *int64  `name:"keep_alive_timeout" json:"keep_alive_timeout,omitempty"`
	MarkCacheSize                     *int64  `name:"mark_cache_size" json:"mark_cache_size,omitempty"`
	MaxConcurrentQueries              *int64  `name:"max_concurrent_queries" json:"max_concurrent_queries,omitempty"`
	MaxConnections                    *int64  `name:"max_connections" json:"max_connections,omitempty"`
	MaxPartitionSizeToDrop            *int64  `name:"max_partition_size_to_drop" json:"max_partition_size_to_drop,omitempty"`
	MaxTableSizeToDrop                *int64  `name:"max_table_size_to_drop" json:"max_table_size_to_drop,omitempty"`
	SSLClientVerificationMode         *string `name:"ssl_client_verification_mode" json:"ssl_client_verification_mode,omitempty"`
	Timezone                          *string `name:"timezone" json:"timezone,omitempty"`
	UncompressedCacheSize             *int64  `name:"uncompressed_cache_size" json:"uncompressed_cache_size,omitempty"`
	AllowPlaintextPassword            *int64  `name:"allow_plaintext_password" json:"allow_plaintext_password,omitempty"`
	AllowNoPassword                   *int64  `name:"allow_no_password" json:"allow_no_password,omitempty"`
}

func (chcfg ClickHouseConfig) ToModel() (chmodels.ClickHouseConfig, error) {
	result := chmodels.ClickHouseConfig{
		Compressions:    compressionsToModel(chcfg.Compressions),
		Dictionaries:    dictionariesToModel(chcfg.Dictionaries),
		GraphiteRollups: graphiteRollupsToModel(chcfg.GraphiteRollups),
		Kafka:           chcfg.Kafka.toModel(),
		KafkaTopics:     kafkaTopicsToModel(chcfg.KafkaTopics),
		RabbitMQ:        chcfg.RabbitMQ.toModel(),
	}

	if err := pillars.MapPillarToModel(&chcfg, &result); err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	return result, nil
}

type MergeTree struct {
	EnableMixedGranularityParts                    *bool  `name:"enable_mixed_granularity_parts" json:"enable_mixed_granularity_parts,omitempty"`
	MaxBytesToMergeAtMinSpaceInPool                *int64 `name:"max_bytes_to_merge_at_min_space_in_pool" json:"max_bytes_to_merge_at_min_space_in_pool,omitempty"`
	MaxBytesToMergeAtMaxSpaceInPool                *int64 `name:"max_bytes_to_merge_at_max_space_in_pool" json:"max_bytes_to_merge_at_max_space_in_pool,omitempty"`
	MaxReplicatedMergesInQueue                     *int64 `name:"max_replicated_merges_in_queue" json:"max_replicated_merges_in_queue,omitempty"`
	NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge *int64 `name:"number_of_free_entries_in_pool_to_lower_max_size_of_merge" json:"number_of_free_entries_in_pool_to_lower_max_size_of_merge,omitempty"`
	PartsToDelayInsert                             *int64 `name:"parts_to_delay_insert" json:"parts_to_delay_insert,omitempty"`
	PartsToThrowInsert                             *int64 `name:"parts_to_throw_insert" json:"parts_to_throw_insert,omitempty"`
	InactivePartsToDelayInsert                     *int64 `name:"inactive_parts_to_delay_insert" json:"inactive_parts_to_delay_insert,omitempty"`
	InactivePartsToThrowInsert                     *int64 `name:"inactive_parts_to_throw_insert" json:"inactive_parts_to_throw_insert,omitempty"`
	ReplicatedDeduplicationWindow                  *int64 `name:"replicated_deduplication_window" json:"replicated_deduplication_window,omitempty"`
	ReplicatedDeduplicationWindowSeconds           *int64 `name:"replicated_deduplication_window_seconds" json:"replicated_deduplication_window_seconds,omitempty"`
}

type Compression struct {
	Method           chmodels.CompressionMethod `json:"method,omitempty"`
	MinPartSize      int64                      `json:"min_part_size,omitempty"`
	MinPartSizeRatio float64                    `json:"min_part_size_ratio,omitempty"`
}

func (c Compression) toModel() chmodels.Compression {
	return chmodels.Compression{
		Method:           c.Method,
		MinPartSize:      c.MinPartSize,
		MinPartSizeRatio: c.MinPartSizeRatio,
	}
}

func compressionsToModel(c []Compression) []chmodels.Compression {
	res := make([]chmodels.Compression, len(c))
	for i, compression := range c {
		res[i] = compression.toModel()
	}
	return res
}

func (c Compression) fromModel(cm chmodels.Compression) Compression {
	return Compression{
		Method:           cm.Method,
		MinPartSize:      cm.MinPartSize,
		MinPartSizeRatio: cm.MinPartSizeRatio,
	}
}

func compressionsFromModel(c []chmodels.Compression) []Compression {
	res := make([]Compression, len(c))
	for i, compression := range c {
		res[i] = Compression{}.fromModel(compression)
	}
	return res
}

type GraphiteRollup struct {
	Name     string                  `json:"name,omitempty"`
	Patterns []GraphiteRollupPattern `json:"patterns,omitempty"`
}

func (gr GraphiteRollup) toModel() chmodels.GraphiteRollup {
	return chmodels.GraphiteRollup{
		Name:     gr.Name,
		Patterns: graphiteRollupPatternsToModel(gr.Patterns),
	}
}

func graphiteRollupFromModel(gr chmodels.GraphiteRollup) GraphiteRollup {
	return GraphiteRollup{
		Name:     gr.Name,
		Patterns: graphiteRollupPatternsFromModel(gr.Patterns),
	}
}

func graphiteRollupsToModel(r []GraphiteRollup) []chmodels.GraphiteRollup {
	res := make([]chmodels.GraphiteRollup, len(r))
	for i, rollup := range r {
		res[i] = rollup.toModel()
	}
	return res
}

func graphiteRollupsFromModel(grs []chmodels.GraphiteRollup) []GraphiteRollup {
	res := make([]GraphiteRollup, 0, len(grs))
	for _, gr := range grs {
		res = append(res, graphiteRollupFromModel(gr))
	}
	return res
}

type GraphiteRollupPattern struct {
	Regexp      *string                          `json:"regexp,omitempty"`
	Function    string                           `json:"function,omitempty"`
	Retentionts []GraphiteRollupPatternRetention `json:"retention,omitempty"`
}

func (grp GraphiteRollupPattern) toModel() chmodels.GraphiteRollupPattern {
	return chmodels.GraphiteRollupPattern{
		Regexp:      pillars.MapPtrStringToOptionalString(grp.Regexp),
		Function:    grp.Function,
		Retentionts: graphiteRollupPatternRetentionsToModel(grp.Retentionts),
	}
}

func graphiteRollupPatternFromModel(grp chmodels.GraphiteRollupPattern) GraphiteRollupPattern {
	return GraphiteRollupPattern{
		Regexp:      pillars.MapOptionalStringToPtrString(grp.Regexp),
		Function:    grp.Function,
		Retentionts: graphiteRollupPatternRetentionsFromModel(grp.Retentionts),
	}
}

func graphiteRollupPatternsToModel(p []GraphiteRollupPattern) []chmodels.GraphiteRollupPattern {
	res := make([]chmodels.GraphiteRollupPattern, len(p))
	for i, pattern := range p {
		res[i] = pattern.toModel()
	}
	return res
}

func graphiteRollupPatternsFromModel(grps []chmodels.GraphiteRollupPattern) []GraphiteRollupPattern {
	res := make([]GraphiteRollupPattern, 0, len(grps))
	for _, grp := range grps {
		res = append(res, graphiteRollupPatternFromModel(grp))
	}
	return res
}

type GraphiteRollupPatternRetention struct {
	Age       int64 `json:"age,omitempty"`
	Precision int64 `json:"precision,omitempty"`
}

func (grpr GraphiteRollupPatternRetention) toModel() chmodels.GraphiteRollupPatternRetention {
	return chmodels.GraphiteRollupPatternRetention{
		Age:       grpr.Age,
		Precision: grpr.Precision,
	}
}

func graphiteRollupPatternRetentionFromModel(grpr chmodels.GraphiteRollupPatternRetention) GraphiteRollupPatternRetention {
	return GraphiteRollupPatternRetention{
		Age:       grpr.Age,
		Precision: grpr.Precision,
	}
}

func graphiteRollupPatternRetentionsToModel(r []GraphiteRollupPatternRetention) []chmodels.GraphiteRollupPatternRetention {
	res := make([]chmodels.GraphiteRollupPatternRetention, len(r))
	for i, retention := range r {
		res[i] = retention.toModel()
	}
	return res
}

func graphiteRollupPatternRetentionsFromModel(grprs []chmodels.GraphiteRollupPatternRetention) []GraphiteRollupPatternRetention {
	res := make([]GraphiteRollupPatternRetention, 0, len(grprs))
	for _, grpr := range grprs {
		res = append(res, graphiteRollupPatternRetentionFromModel(grpr))
	}
	return res
}

type Kafka struct {
	SecurityProtocol *string            `json:"security_protocol,omitempty"`
	SaslMechanism    *string            `json:"sasl_mechanism,omitempty"`
	SaslUsername     *string            `json:"sasl_username,omitempty"`
	SaslPassword     *pillars.CryptoKey `json:"sasl_password,omitempty"`
}

func (kf *Kafka) toModel() chmodels.Kafka {
	if kf == nil {
		return chmodels.Kafka{}
	}

	return chmodels.Kafka{
		SecurityProtocol: chmodels.KafkaSecurityProtocol(pillars.MapPtrStringToOptionalString(kf.SecurityProtocol)),
		SaslMechanism:    chmodels.KafkaSaslMechanism(pillars.MapPtrStringToOptionalString(kf.SaslMechanism)),
		SaslUsername:     pillars.MapPtrStringToOptionalString(kf.SaslUsername),
		Valid:            true,
	}
}

type KafkaTopic struct {
	Name     string `json:"name,omitempty"`
	Settings Kafka  `json:"settings,omitempty"`
}

func (kft KafkaTopic) toModel() chmodels.KafkaTopic {
	return chmodels.KafkaTopic{
		Name:     kft.Name,
		Settings: kft.Settings.toModel(),
	}
}

func kafkaFromModel(cryptoProvider crypto.Crypto, m chmodels.Kafka) (Kafka, error) {
	var kf Kafka
	pass, err := EncryptOptionalPassword(cryptoProvider, m.SaslPassword)
	if err != nil {
		return kf, err
	}
	kf.SaslPassword = pass
	kf.SecurityProtocol = pillars.MapOptionalStringToPtrString(optional.String(m.SecurityProtocol))
	kf.SaslMechanism = pillars.MapOptionalStringToPtrString(optional.String(m.SaslMechanism))
	kf.SaslUsername = pillars.MapOptionalStringToPtrString(m.SaslUsername)
	return kf, nil
}

func kafkaTopicsToModel(kfts []KafkaTopic) []chmodels.KafkaTopic {
	res := make([]chmodels.KafkaTopic, len(kfts))
	for i, topic := range kfts {
		res[i] = topic.toModel()
	}
	return res
}

func kafkaTopicFromModel(cryptoProvider crypto.Crypto, m chmodels.KafkaTopic) (KafkaTopic, error) {
	kafka, err := kafkaFromModel(cryptoProvider, m.Settings)
	if err != nil {
		return KafkaTopic{}, err
	}
	return KafkaTopic{
		Name:     m.Name,
		Settings: kafka,
	}, nil
}

func kafkaTopicsFromModel(cryptoProvider crypto.Crypto, kftms []chmodels.KafkaTopic) ([]KafkaTopic, error) {
	var (
		res = make([]KafkaTopic, len(kftms))
		err error
	)

	for i, topic := range kftms {
		res[i], err = kafkaTopicFromModel(cryptoProvider, topic)
		if err != nil {
			return nil, err
		}
	}

	return res, nil
}

type RabbitMQ struct {
	Username *string            `json:"username,omitempty"`
	Password *pillars.CryptoKey `json:"password,omitempty"`
}

func (rmq *RabbitMQ) toModel() chmodels.RabbitMQ {
	if rmq == nil {
		return chmodels.RabbitMQ{}
	}

	return chmodels.RabbitMQ{
		Username: pillars.MapPtrStringToOptionalString(rmq.Username),
		Valid:    true,
	}
}

func rabbitMQFromModel(cryptoProvider crypto.Crypto, m chmodels.RabbitMQ) (RabbitMQ, error) {
	pass, err := EncryptOptionalPassword(cryptoProvider, m.Password)
	if err != nil {
		return RabbitMQ{}, err
	}
	return RabbitMQ{
		Username: pillars.MapOptionalStringToPtrString(m.Username),
		Password: pass,
	}, nil
}

func (p *SubClusterCH) SetClickHouseConfig(cryptoProvider crypto.Crypto, config chmodels.ClickHouseConfig) error {
	if err := pillars.MapModelToPillar(&config, &p.Data.ClickHouse.Config); err != nil {
		return err
	}
	if err := pillars.MapModelToPillar(&config.MergeTree, &p.Data.ClickHouse.Config.MergeTree); err != nil {
		return err
	}

	{
		compressions := compressionsFromModel(config.Compressions)
		p.Data.ClickHouse.Config.Compressions = append(p.Data.ClickHouse.Config.Compressions, compressions...)
	}
	{
		dictionaries, err := dictionariesFromModel(cryptoProvider, config.Dictionaries)
		if err != nil {
			return err
		}
		p.Data.ClickHouse.Config.Dictionaries = append(p.Data.ClickHouse.Config.Dictionaries, dictionaries...)
	}
	{
		rollups := graphiteRollupsFromModel(config.GraphiteRollups)
		p.Data.ClickHouse.Config.GraphiteRollups = append(p.Data.ClickHouse.Config.GraphiteRollups, rollups...)
	}
	{
		topics, err := kafkaTopicsFromModel(cryptoProvider, config.KafkaTopics)
		if err != nil {
			return err
		}
		p.Data.ClickHouse.Config.KafkaTopics = append(p.Data.ClickHouse.Config.KafkaTopics, topics...)
	}

	if config.Kafka.Valid {
		kafka, err := kafkaFromModel(cryptoProvider, config.Kafka)
		if err != nil {
			return err
		}

		p.Data.ClickHouse.Config.Kafka = &kafka
	}
	if config.RabbitMQ.Valid {
		rmq, err := rabbitMQFromModel(cryptoProvider, config.RabbitMQ)
		if err != nil {
			return err
		}

		p.Data.ClickHouse.Config.RabbitMQ = &rmq
	}

	return nil
}

func EncryptOptionalPassword(cryptoProvider crypto.Crypto, password optional.OptionalPassword) (*pillars.CryptoKey, error) {
	if !password.Valid {
		return nil, nil
	}

	key, err := cryptoProvider.Encrypt([]byte(password.Password.Unmask()))
	return &key, err
}
