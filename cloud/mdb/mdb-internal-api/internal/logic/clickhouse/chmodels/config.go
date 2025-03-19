package chmodels

import (
	"reflect"
	"time"

	"github.com/mitchellh/copystructure"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type commonClusterSpec struct {
	Version             string
	ClickHouseResources models.ClusterResourcesSpec
	Access              clusters.Access
	Encryption          clusters.Encryption
}

type onlyMDBClusterSpec struct {
	ZookeeperResources    models.ClusterResourcesSpec
	BackupWindowStart     backups.OptionalBackupWindowStart
	AdminPassword         optional.OptionalPassword
	SQLUserManagement     optional.Bool
	SQLDatabaseManagement optional.Bool
	EmbeddedKeeper        bool
	MySQLProtocol         optional.Bool
	PostgreSQLProtocol    optional.Bool
	EnableCloudStorage    optional.Bool
	Config                ClickHouseConfig
	CloudStorageConfig    CloudStorageConfig
}

type onlyDataCloudClusterSpec struct {
}

type CombinedClusterSpec struct {
	commonClusterSpec
	onlyMDBClusterSpec
	onlyDataCloudClusterSpec
}

type ClusterConfig struct {
	Version               string
	ClickhouseResources   models.ClusterResources
	ZooKeeperResources    models.ClusterResources
	BackupWindowStart     backups.BackupWindowStart
	Access                clusters.Access
	CloudStorageEnabled   bool
	SQLDatabaseManagement bool
	SQLUserManagement     bool
	EmbeddedKeeper        bool
	MySQLProtocol         bool
	PostgreSQLProtocol    bool

	ClickhouseConfigSet ClickhouseConfigSet

	CloudStorageConfig CloudStorageConfig
}

type ClickhouseConfigSet struct {
	Default   ClickHouseConfig
	User      ClickHouseConfig
	Effective ClickHouseConfig
}

type CloudStorageConfig struct {
	DataCacheEnabled optional.Bool
	DataCacheMaxSize optional.Int64
	MoveFactor       optional.Float64
}

func (cfg CloudStorageConfig) Valid() bool {
	return cfg.DataCacheEnabled.Valid || cfg.DataCacheMaxSize.Valid || cfg.MoveFactor.Valid
}

type MDBClusterSpec struct {
	commonClusterSpec
	onlyMDBClusterSpec
}

func (cs MDBClusterSpec) Combine() CombinedClusterSpec {
	return CombinedClusterSpec{
		commonClusterSpec:  cs.commonClusterSpec,
		onlyMDBClusterSpec: cs.onlyMDBClusterSpec,
	}
}

type DataCloudClusterSpec struct {
	commonClusterSpec
	onlyDataCloudClusterSpec
}

func (cs DataCloudClusterSpec) Combine() CombinedClusterSpec {
	return CombinedClusterSpec{
		commonClusterSpec:        cs.commonClusterSpec,
		onlyDataCloudClusterSpec: cs.onlyDataCloudClusterSpec,
	}
}

type commonRestoreClusterSpec struct {
	Version        string
	ShardResources []models.ClusterResourcesSpec
	Access         clusters.Access
}

type RestoreMDBClusterSpec struct {
	commonRestoreClusterSpec
	onlyMDBClusterSpec
}

func (cs RestoreMDBClusterSpec) Combine() CombinedClusterSpec {
	return CombinedClusterSpec{
		commonClusterSpec: commonClusterSpec{
			Version:             cs.Version,
			ClickHouseResources: cs.ShardResources[0],
			Access:              cs.Access,
		},
		onlyMDBClusterSpec: cs.onlyMDBClusterSpec,
	}
}

type ClickHouseConfig struct {
	MergeTree MergeTreeConfig `mapping:"map"`

	Compressions    []Compression    `mapping:"omit"`
	Dictionaries    []Dictionary     `mapping:"omit"`
	GraphiteRollups []GraphiteRollup `mapping:"omit"`
	Kafka           Kafka            `mapping:"omit"`
	KafkaTopics     []KafkaTopic     `mapping:"omit"`
	RabbitMQ        RabbitMQ         `mapping:"omit"`

	BackgroundPoolSize                    optional.Int64
	BackgroundFetchesPoolSize             optional.Int64
	BackgroundSchedulePoolSize            optional.Int64
	BackgroundMovePoolSize                optional.Int64
	BackgroundDistributedSchedulePoolSize optional.Int64
	BackgroundBufferFlushSchedulePoolSize optional.Int64

	LogLevel                    ClickHouseLogLevel
	QueryLogRetentionSize       optional.Int64
	QueryLogRetentionTime       optional.Int64
	QueryThreadLogEnabled       optional.Bool
	QueryThreadLogRetentionSize optional.Int64
	QueryThreadLogRetentionTime optional.Int64
	PartLogRetentionSize        optional.Int64
	PartLogRetentionTime        optional.Int64
	MetricLogEnabled            optional.Bool
	MetricLogRetentionSize      optional.Int64
	MetricLogRetentionTime      optional.Int64
	TraceLogEnabled             optional.Bool
	TraceLogRetentionSize       optional.Int64
	TraceLogRetentionTime       optional.Int64
	TextLogEnabled              optional.Bool
	TextLogRetentionSize        optional.Int64
	TextLogRetentionTime        optional.Int64
	TextLogLevel                ClickHouseLogLevel
	OpenTelemetrySpanLogEnabled optional.Bool

	BuiltinDictionariesReloadInterval optional.Int64
	DefaultDatabase                   optional.String
	DefaultProfile                    optional.String
	GeobaseURI                        optional.String
	KeepAliveTimeout                  optional.Int64
	MarkCacheSize                     optional.Int64
	MaxConcurrentQueries              optional.Int64
	MaxConnections                    optional.Int64
	MaxPartitionSizeToDrop            optional.Int64
	MaxTableSizeToDrop                optional.Int64
	SSLClientVerificationMode         optional.String
	Timezone                          optional.String
	UncompressedCacheSize             optional.Int64
}

type MergeTreeConfig struct {
	EnableMixedGranularityParts                    optional.Bool
	MaxBytesToMergeAtMinSpaceInPool                optional.Int64
	MaxBytesToMergeAtMaxSpaceInPool                optional.Int64
	MaxReplicatedMergesInQueue                     optional.Int64
	NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge optional.Int64
	PartsToDelayInsert                             optional.Int64
	PartsToThrowInsert                             optional.Int64
	InactivePartsToDelayInsert                     optional.Int64
	InactivePartsToThrowInsert                     optional.Int64
	ReplicatedDeduplicationWindow                  optional.Int64
	ReplicatedDeduplicationWindowSeconds           optional.Int64
}

type ClickHouseLogLevel optional.String

var (
	LogLevelUnknown     = ClickHouseLogLevel(optional.String{})
	LogLevelTrace       = ClickHouseLogLevel(optional.NewString("trace"))
	LogLevelDebug       = ClickHouseLogLevel(optional.NewString("debug"))
	LogLevelInformation = ClickHouseLogLevel(optional.NewString("information"))
	LogLevelWarning     = ClickHouseLogLevel(optional.NewString("warning"))
	LogLevelError       = ClickHouseLogLevel(optional.NewString("error"))
)

func mergeLogLevels(base ClickHouseLogLevel, add ClickHouseLogLevel) ClickHouseLogLevel {
	if add != LogLevelUnknown {
		return add
	}
	return base
}

type Compression struct {
	Method           CompressionMethod
	MinPartSize      int64
	MinPartSizeRatio float64
}

type CompressionMethod string

const (
	CompressionMethodLZ4  = CompressionMethod("lz4")
	CompressionMethodZSTD = CompressionMethod("zstd")
)

type GraphiteRollup struct {
	Name     string
	Patterns []GraphiteRollupPattern
}

type GraphiteRollupPattern struct {
	Regexp      optional.String
	Function    string
	Retentionts []GraphiteRollupPatternRetention
}

type GraphiteRollupPatternRetention struct {
	Age       int64
	Precision int64
}

type Kafka struct {
	SecurityProtocol KafkaSecurityProtocol
	SaslMechanism    KafkaSaslMechanism
	SaslUsername     optional.String
	SaslPassword     optional.OptionalPassword

	Valid bool
}

type KafkaSecurityProtocol optional.String

var (
	KafkaSecurityProtocolUnspecified   = KafkaSecurityProtocol{}
	KafkaSecurityProtocolPlainText     = KafkaSecurityProtocol(optional.NewString("PLAINTEXT"))
	KafkaSecurityProtocolSSL           = KafkaSecurityProtocol(optional.NewString("SSL"))
	KafkaSecurityProtocolSaslPlainText = KafkaSecurityProtocol(optional.NewString("SASL_PLAINTEXT"))
	KafkaSecurityProtocolSaslSSL       = KafkaSecurityProtocol(optional.NewString("SASL_SSL"))
)

type KafkaSaslMechanism optional.String

var (
	KafkaSaslMechanismUnspecified = KafkaSaslMechanism{}
	KafkaSaslMechanismGssAPI      = KafkaSaslMechanism(optional.NewString("GSSAPI"))
	KafkaSaslMechanismPlain       = KafkaSaslMechanism(optional.NewString("PLAIN"))
	KafkaSaslMechanismScramSHA256 = KafkaSaslMechanism(optional.NewString("SCRAM-SHA-256"))
	KafkaSaslMechanismScramSHA512 = KafkaSaslMechanism(optional.NewString("SCRAM-SHA-512"))
)

type KafkaTopic struct {
	Name     string
	Settings Kafka
}

type RabbitMQ struct {
	Username optional.String
	Password optional.OptionalPassword

	Valid bool
}

// mergeOptionalFieldsOfStructs overrides base's optional fields with add's optional fields if latter are valid.
// Both base and add must be pointers to same struct objects.
// Returns a pointer to struct interface of the same type as base and add. Result is base's fields overriden by valid add's fields
func mergeOptionalFieldsOfStructs(base, add interface{}) (interface{}, error) {
	res, err := copystructure.Copy(base)
	if err != nil {
		return nil, err
	}

	resVal := reflect.ValueOf(res)
	addVal := reflect.ValueOf(add)

	if resVal.Kind() != reflect.Ptr {
		return nil, xerrors.Errorf("base argument is not a pointer")
	}
	if addVal.Kind() != reflect.Ptr {
		return nil, xerrors.Errorf("add argument is not a pointer")
	}
	resVal = resVal.Elem()
	addVal = addVal.Elem()
	if resVal.Kind() != reflect.Struct {
		return nil, xerrors.Errorf("provided base value is not a struct: %+v", resVal)
	}
	if addVal.Kind() != reflect.Struct {
		return nil, xerrors.Errorf("provided add value is not a struct: %+v", addVal)
	}
	typeOfBase := resVal.Type()
	typeOfAdd := addVal.Type()
	if typeOfBase != typeOfAdd {
		return nil, xerrors.Errorf("base and add arguments should have the same type, actual base type: %+v, and add type: %+v", resVal.Type(), addVal.Type())
	}

	optBoolType := reflect.TypeOf(optional.Bool{})
	optInt64Type := reflect.TypeOf(optional.Int64{})
	optStringType := reflect.TypeOf(optional.String{})

	for i := 0; i < addVal.NumField(); i++ {
		field := addVal.Field(i)
		elemType := field.Type()
		if elemType == optBoolType || elemType == optInt64Type || elemType == optStringType {
			if field.FieldByName("Valid").Bool() {
				resVal.Field(i).Set(field)
			}
		}
	}

	return res, nil
}

func MergeClickhouseConfigs(base ClickHouseConfig, add ClickHouseConfig) (ClickHouseConfig, error) {
	tmp, err := mergeOptionalFieldsOfStructs(&base, &add)
	if err != nil {
		return ClickHouseConfig{}, err
	}
	res := *tmp.(*ClickHouseConfig)

	tmp, err = mergeOptionalFieldsOfStructs(&base.MergeTree, &add.MergeTree)
	if err != nil {
		return ClickHouseConfig{}, err
	}
	res.MergeTree = *tmp.(*MergeTreeConfig)

	res.LogLevel = mergeLogLevels(res.LogLevel, add.LogLevel)
	res.TextLogLevel = mergeLogLevels(res.TextLogLevel, add.TextLogLevel)

	res.Kafka = add.Kafka
	res.RabbitMQ = add.RabbitMQ
	res.Compressions = add.Compressions
	res.Dictionaries = add.Dictionaries
	res.GraphiteRollups = add.GraphiteRollups
	res.KafkaTopics = add.KafkaTopics

	return res, nil
}

func (c ClickHouseConfig) ValidateAndSane() error {

	if c.MaxConnections.Valid && c.MaxConnections.Int64 < 10 {
		return semerr.InvalidInputf("invalid max_connections value %d: must be 10 or greater", c.MaxConnections.Int64)
	}
	if c.MaxConcurrentQueries.Valid && c.MaxConcurrentQueries.Int64 < 60 {
		return semerr.InvalidInputf("invalid max_concurrent_queries value %d: must be 10 or greater", c.MaxConcurrentQueries.Int64)
	}
	if c.MarkCacheSize.Valid && c.MarkCacheSize.Int64 < 1 {
		return semerr.InvalidInputf("invalid mark_cache_size value %d: must be 1 or greater", c.MaxConcurrentQueries.Int64)
	}

	if c.Timezone.Valid {
		if _, err := time.LoadLocation(c.Timezone.String); err != nil {
			return semerr.InvalidInputf("invalid time_zone value %s: %q", c.Timezone.String, err)
		}
	}

	if c.QueryLogRetentionTime.Valid && c.QueryLogRetentionTime.Int64 < 1 {
		return semerr.InvalidInputf("invalid query_log_retention_time value %d: must be at least 1 second", c.QueryLogRetentionTime.Int64)
	}
	if c.QueryThreadLogRetentionTime.Valid && c.QueryThreadLogRetentionTime.Int64 < 1 {
		return semerr.InvalidInputf("invalid query_thread_log_retention_time value %d: must be at least 1 second", c.QueryThreadLogRetentionTime.Int64)
	}
	if c.PartLogRetentionTime.Valid && c.PartLogRetentionTime.Int64 < 1 {
		return semerr.InvalidInputf("invalid part_log_retention_time value %d: must be at least 1 second", c.PartLogRetentionTime.Int64)
	}
	if c.MetricLogRetentionTime.Valid && c.MetricLogRetentionTime.Int64 < 1 {
		return semerr.InvalidInputf("invalid metric_log_retention_time value %d: must be at least 1 second", c.MetricLogRetentionTime.Int64)
	}
	if c.TraceLogRetentionTime.Valid && c.TraceLogRetentionTime.Int64 < 1 {
		return semerr.InvalidInputf("invalid trace_log_retention_time value %d: must be at least 1 second", c.TraceLogRetentionTime.Int64)
	}
	if c.TextLogRetentionTime.Valid && c.TextLogRetentionTime.Int64 < 1 {
		return semerr.InvalidInputf("invalid text_log_retention_time value %d: must be at least 1 second", c.TextLogRetentionTime.Int64)
	}

	var backgroundPoolSize int64 = 16
	var numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge int64 = 8
	if c.MergeTree.NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge.Valid {
		numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge = c.MergeTree.NumberOfFreeEntriesInPoolToLowerMaxSizeOfMerge.Int64
	}
	if c.BackgroundPoolSize.Valid {
		backgroundPoolSize = c.BackgroundPoolSize.Int64
	}

	if backgroundPoolSize < numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge {
		return semerr.InvalidInputf("value of merge_tree.number_of_free_entries_in_pool_to_lower_max_size_of_merge (%d)"+
			" greater than background_pool_size (%d)", numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge, backgroundPoolSize)
	}

	for _, dict := range c.Dictionaries {
		if err := dict.Validate(); err != nil {
			return err
		}
	}

	return c.MergeTree.Validate()
}

func (c MergeTreeConfig) Validate() error {
	return nil
}
