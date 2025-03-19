package chmodels

import (
	"time"

	fieldmask_utils "github.com/mennanov/fieldmask-utils"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

type User struct {
	ClusterID   string
	Name        string
	Permissions []Permission
	Settings    UserSettings
	UserQuotas  []UserQuota
}

type UserSpec struct {
	Name        string
	Password    secret.String
	Permissions []Permission
	Settings    UserSettings
	UserQuotas  []UserQuota
}

type UpdateUserArgs struct {
	Password     *secret.String
	Permissions  *[]Permission
	UserQuotas   *[]UserQuota
	Settings     *UserSettings
	SettingsMask fieldmask_utils.FieldFilter
}

type Permission struct {
	DatabaseName string
	DataFilters  []DataFilter
}

type UserQuota struct {
	IntervalDuration time.Duration
	Queries          optional.Int64
	Errors           optional.Int64
	ResultRows       optional.Int64
	ReadRows         optional.Int64
	ExecutionTime    optional.Duration
}

type DataFilter struct {
	TableName string
	Filter    string
}

type UserSettings struct {
	// Permissions
	Readonly optional.Int64
	AllowDDL optional.Int64

	// Timeouts
	ConnectTimeout optional.Int64
	ReceiveTimeout optional.Int64
	SendTimeout    optional.Int64

	// Replication settings
	InsertQuorum                   optional.Int64
	InsertQuorumTimeout            optional.Int64
	InsertQuorumParallel           optional.Bool
	SelectSequentialConsistency    optional.Int64
	ReplicationAlterPartitionsSync optional.Int64

	// Settings of distributed queries
	MaxReplicaDelayForDistributedQueries         optional.Int64
	FallbackToStaleReplicasForDistributedQueries optional.Int64
	DistributedProductMode                       DistributedProductMode
	DistributedAggregationMemoryEfficient        optional.Int64
	DistributedDDLTaskTimeout                    optional.Int64
	SkipUnavailableShards                        optional.Int64
	LoadBalancing                                LoadBalancing

	// Query and expression compilations
	Compile                     optional.Int64
	MinCountToCompile           optional.Int64
	CompileExpressions          optional.Int64
	MinCountToCompileExpression optional.Int64
	OptimizeMoveToPrewhere      optional.Int64

	// I/O settings
	MaxBlockSize                                  optional.Int64
	MinInsertBlockSizeRows                        optional.Int64
	MinInsertBlockSizeBytes                       optional.Int64
	MaxInsertBlockSize                            optional.Int64
	MaxPartitionsPerInsertBlock                   optional.Int64
	MinBytesToUseDirectIO                         optional.Int64
	UseUncompressedCache                          optional.Int64
	MergeTreeMaxRowsToUseCache                    optional.Int64
	MergeTreeMaxBytesToUseCache                   optional.Int64
	MergeTreeMinRowsForConcurrentRead             optional.Int64
	MergeTreeMinBytesForConcurrentRead            optional.Int64
	MaxBytesBeforeExternalGroupBy                 optional.Int64
	MaxBytesBeforeExternalSort                    optional.Int64
	GroupByTwoLevelThreshold                      optional.Int64
	GroupByTwoLevelThresholdBytes                 optional.Int64
	DeduplicateBlocksInDependentMaterializedViews optional.Bool

	// Resource usage limits and query priorities
	Priority                    optional.Int64
	MaxThreads                  optional.Int64
	MaxMemoryUsage              optional.Int64
	MaxMemoryUsageForUser       optional.Int64
	MaxNetworkBandwidth         optional.Int64
	MaxNetworkBandwidthForUser  optional.Int64
	MaxConcurrentQueriesForUser optional.Int64

	// Query complexity limits
	ForceIndexByDate            optional.Int64
	ForcePrimaryKey             optional.Int64
	MaxRowsToRead               optional.Int64
	MaxBytesToRead              optional.Int64
	ReadOverflowMode            OverflowMode
	MaxRowsToGroupBy            optional.Int64
	GroupByOverflowMode         GroupByOverflowMode
	MaxRowsToSort               optional.Int64
	MaxBytesToSort              optional.Int64
	SortOverflowMode            OverflowMode
	MaxResultRows               optional.Int64
	MaxResultBytes              optional.Int64
	ResultOverflowMode          OverflowMode
	MaxRowsInDistinct           optional.Int64
	MaxBytesInDistinct          optional.Int64
	DistinctOverflowMode        OverflowMode
	MaxRowsToTransfer           optional.Int64
	MaxBytesToTransfer          optional.Int64
	TransferOverflowMode        OverflowMode
	MaxExecutionTime            optional.Int64
	TimeoutOverflowMode         OverflowMode
	MaxRowsInSet                optional.Int64
	MaxBytesInSet               optional.Int64
	SetOverflowMode             OverflowMode
	MaxRowsInJoin               optional.Int64
	MaxBytesInJoin              optional.Int64
	JoinOverflowMode            OverflowMode
	MaxColumnsToRead            optional.Int64
	MaxTemporaryColumns         optional.Int64
	MaxTemporaryNonConstColumns optional.Int64
	MaxQuerySize                optional.Int64
	MaxAstDepth                 optional.Int64
	MaxAstElements              optional.Int64
	MaxExpandedAstElements      optional.Int64
	MinExecutionSpeed           optional.Int64
	MinExecutionSpeedBytes      optional.Int64

	// Settings of input and output formats
	InputFormatValuesInterpretExpressions optional.Int64
	InputFormatDefaultsForOmittedFields   optional.Int64
	InputFormatWithNamesUseHeader         optional.Bool
	InputFormatNullAsDefault              optional.Bool
	OutputFormatJSONQuote64bitIntegers    optional.Int64
	OutputFormatJSONQuoteDenormals        optional.Int64
	LowCardinalityAllowInNativeFormat     optional.Int64
	EmptyResultForAggregationByEmptySet   optional.Int64
	DateTimeInputFormat                   DateTimeInputFormat
	DateTimeOutputFormat                  DateTimeOutputFormat
	FormatRegexp                          string
	FormatRegexpEscapingRule              FormatRegexpEscapingRule
	FormatRegexpSkipUnmatched             optional.Bool

	// HTTP-specific settings
	HTTPConnectionTimeout       optional.Int64
	HTTPReceiveTimeout          optional.Int64
	HTTPSendTimeout             optional.Int64
	EnableHTTPCompression       optional.Int64
	SendProgressInHTTPHeaders   optional.Int64
	HTTPHeadersProgressInterval optional.Int64
	AddHTTPCorsHeader           optional.Int64

	// Quoting settings
	QuotaMode QuotaMode

	// Other settings
	CountDistinctImplementation   CountDistinctImplementation
	JoinAlgorithm                 JoinAlgorithm
	AnyJoinDistinctRightTableKeys optional.Bool
	JoinedSubqueryRequiresAlias   optional.Int64
	JoinUseNulls                  optional.Int64
	TransformNullIn               optional.Int64
}

type OverflowMode string

const (
	OverflowModeUnspecified OverflowMode = ""
	OverflowModeThrow       OverflowMode = "throw"
	OverflowModeBreak       OverflowMode = "break"
)

type GroupByOverflowMode string

const (
	GroupByOverflowModeUnspecified GroupByOverflowMode = ""
	GroupByOverflowModeThrow       GroupByOverflowMode = "throw"
	GroupByOverflowModeBreak       GroupByOverflowMode = "break"
	GroupByOverflowModeAny         GroupByOverflowMode = "any"
)

type DistributedProductMode string

const (
	DistributedProductModeUnspecified DistributedProductMode = ""
	DistributedProductModeDeny        DistributedProductMode = "deny"
	DistributedProductModeLocal       DistributedProductMode = "local"
	DistributedProductModeGlobal      DistributedProductMode = "global"
	DistributedProductModeAllow       DistributedProductMode = "allow"
)

type QuotaMode string

const (
	QuotaModeUnspecified QuotaMode = ""
	QuotaModeDefault     QuotaMode = "default"
	QuotaModeKeyed       QuotaMode = "keyed"
	QuotaModeKeyedByIP   QuotaMode = "keyed_by_ip"
)

type CountDistinctImplementation string

const (
	CountDistinctImplementationUnspecified    CountDistinctImplementation = ""
	CountDistinctImplementationUniq           CountDistinctImplementation = "uniq"
	CountDistinctImplementationUniqCombined   CountDistinctImplementation = "uniqCombined"
	CountDistinctImplementationUniqCombined64 CountDistinctImplementation = "uniqCombined64"
	CountDistinctImplementationUniqHLL12      CountDistinctImplementation = "uniqHLL12"
	CountDistinctImplementationUniqExact      CountDistinctImplementation = "uniqxact"
)

type LoadBalancing string

const (
	LoadBalancingUnspecified     LoadBalancing = ""
	LoadBalancingRandom          LoadBalancing = "random"
	LoadBalancingNearestHostname LoadBalancing = "nearest_hostname"
	LoadBalancingInOrder         LoadBalancing = "in_order"
	LoadBalancingFirstOrRandom   LoadBalancing = "first_or_random"
	LoadBalancingRoundRobin      LoadBalancing = "round_robin"
)

type DateTimeInputFormat string

const (
	DateTimeInputFormatUnspecified DateTimeInputFormat = ""
	DateTimeInputFormatBestEffort  DateTimeInputFormat = "best_effort"
	DateTimeInputFormatBasic       DateTimeInputFormat = "basic"
)

type DateTimeOutputFormat string

const (
	DateTimeOutputFormatUnspecified DateTimeOutputFormat = ""
	DateTimeOutputFormatBestEffort  DateTimeOutputFormat = "best_effort"
	DateTimeOutputFormatBasic       DateTimeOutputFormat = "basic"
)

type FormatRegexpEscapingRule string

const (
	FormatRegexpEscapingRuleUnspecified FormatRegexpEscapingRule = ""
	FormatRegexpEscapingRuleEscaped     FormatRegexpEscapingRule = "Escaped"
	FormatRegexpEscapingRuleQuoted      FormatRegexpEscapingRule = "Quoted"
	FormatRegexpEscapingRuleCSV         FormatRegexpEscapingRule = "CSV"
	FormatRegexpEscapingRuleJSON        FormatRegexpEscapingRule = "JSON"
	FormatRegexpEscapingRuleRaw         FormatRegexpEscapingRule = "Raw"
)

type JoinAlgorithm string

const (
	JoinAlgorithmUnspecified        JoinAlgorithm = ""
	JoinAlgorithmAuto               JoinAlgorithm = "auto"
	JoinAlgorithmHash               JoinAlgorithm = "hash"
	JoinAlgorithmPartialMerge       JoinAlgorithm = "partial_merge"
	JoinAlgorithmPreferPartialMerge JoinAlgorithm = "prefer_partial_merge"
)

func (us UserSpec) Validate() error {
	if err := userNameValidator.ValidateString(us.Name); err != nil {
		return err
	}

	if err := validateUserPassword(us.Password); err != nil {
		return err
	}

	if err := us.Settings.Validate(); err != nil {
		return err
	}

	if err := validateUserQuotas(us.UserQuotas); err != nil {
		return err
	}

	return nil
}

func (u UpdateUserArgs) Validate() error {
	if u.Password != nil {
		if err := validateUserPassword(*u.Password); err != nil {
			return err
		}
	}

	if u.UserQuotas != nil {
		if err := validateUserQuotas(*u.UserQuotas); err != nil {
			return err
		}
	}

	if u.Settings != nil {
		if err := u.Settings.Validate(); err != nil {
			return err
		}
	}

	return nil
}

func validateUserQuotas(quotas []UserQuota) error {
	quotaIntervals := map[time.Duration]struct{}{}
	for _, quota := range quotas {
		if _, ok := quotaIntervals[quota.IntervalDuration]; ok {
			return semerr.InvalidInput("multiple occurrence of the same IntervalDuration is not allowed")
		}

		quotaIntervals[quota.IntervalDuration] = struct{}{}
	}

	return nil
}

func (s UserSettings) Validate() error {
	// Permissions
	if s.Readonly.Valid && (s.Readonly.Must() < 0 || s.Readonly.Must() > 2) {
		return semerr.InvalidInputf("invalid readonly value %d: must be in range 0-2", s.Readonly.Must())
	}

	// Timeouts
	if s.ConnectTimeout.Valid && s.ConnectTimeout.Must() < 1 {
		return semerr.InvalidInput(
			"invalid connect_timeout value: must be at least 1000 (1 second)")
	}
	if s.ReceiveTimeout.Valid && s.ReceiveTimeout.Must() < 1 {
		return semerr.InvalidInput(
			"invalid receive_timeout value: must be at least 1000 (1 second)")
	}
	if s.SendTimeout.Valid && s.SendTimeout.Must() < 1 {
		return semerr.InvalidInput(
			"invalid send_timeout value: must be at least 1000 (1 second)")
	}

	// Replication settings
	if s.InsertQuorumTimeout.Valid && s.InsertQuorumTimeout.Must() < 1 {
		return semerr.InvalidInput(
			"invalid insert_quorum_timeout value: must be at least 1000 (1 second)")
	}
	if s.ReplicationAlterPartitionsSync.Valid &&
		(s.ReplicationAlterPartitionsSync.Must() < 0 || s.ReplicationAlterPartitionsSync.Must() > 2) {
		return semerr.InvalidInputf(
			"invalid replication_alter_partitions_sync value %d: must be in range 0-2",
			s.ReplicationAlterPartitionsSync.Must())
	}

	// Settings of distributed queries
	if s.MaxReplicaDelayForDistributedQueries.Valid && s.MaxReplicaDelayForDistributedQueries.Must() < 1 {
		return semerr.InvalidInput(
			"invalid max_replica_delay_for_distributed_queries value: must be at least 1000 (1 second)")
	}

	// Query and expression compilations
	if s.MinCountToCompile.Valid && s.MinCountToCompile.Must() < 0 {
		return semerr.InvalidInputf("invalid min_count_to_compile value %d: must be non-negative",
			s.MinCountToCompile.Must())
	}
	if s.MinCountToCompileExpression.Valid && s.MinCountToCompileExpression.Must() < 0 {
		return semerr.InvalidInputf("invalid min_count_to_compile_expression value %d: must be non-negative",
			s.MinCountToCompileExpression.Must())
	}

	// I/O settings
	if s.MaxBlockSize.Valid && s.MaxBlockSize.Must() < 1 {
		return semerr.InvalidInputf("invalid max_block_size value %d: must be positive",
			s.MaxBlockSize.Must())
	}
	if s.MinInsertBlockSizeRows.Valid && s.MinInsertBlockSizeRows.Must() < 0 {
		return semerr.InvalidInputf("invalid min_insert_block_size_rows value %d: must be non-negative",
			s.MinInsertBlockSizeRows.Must())
	}
	if s.MinInsertBlockSizeBytes.Valid && s.MinInsertBlockSizeBytes.Must() < 0 {
		return semerr.InvalidInputf("invalid min_insert_block_size_bytes value %d: must be non-negative",
			s.MinInsertBlockSizeBytes.Must())
	}
	if s.MaxInsertBlockSize.Valid && s.MaxInsertBlockSize.Must() < 1 {
		return semerr.InvalidInputf(
			"invalid max_insert_block_size value %d: must be positive",
			s.MaxInsertBlockSize.Must())
	}
	if s.MinBytesToUseDirectIO.Valid && s.MinBytesToUseDirectIO.Must() < 0 {
		return semerr.InvalidInputf("invalid min_bytes_to_use_direct_io value %d: must be non-negative",
			s.MinBytesToUseDirectIO.Must())
	}
	if s.MergeTreeMaxRowsToUseCache.Valid && s.MergeTreeMaxRowsToUseCache.Must() < 1 {
		return semerr.InvalidInputf(
			"invalid merge_tree_max_rows_to_use_cache %d: must be positive",
			s.MergeTreeMaxRowsToUseCache.Must())
	}
	if s.MergeTreeMaxBytesToUseCache.Valid && s.MergeTreeMaxBytesToUseCache.Must() < 1 {
		return semerr.InvalidInputf(
			"invalid merge_tree_max_bytes_to_use_cache %d: must be positive",
			s.MergeTreeMaxBytesToUseCache.Must())
	}
	if s.MergeTreeMinRowsForConcurrentRead.Valid && s.MergeTreeMinRowsForConcurrentRead.Must() < 1 {
		return semerr.InvalidInputf(
			"invalid merge_tree_min_rows_for_concurrent_read %d: must be positive",
			s.MergeTreeMinRowsForConcurrentRead.Must())
	}
	if s.MergeTreeMinBytesForConcurrentRead.Valid && s.MergeTreeMinBytesForConcurrentRead.Must() < 1 {
		return semerr.InvalidInputf(
			"invalid merge_tree_min_bytes_for_concurrent_read %d: must be positive",
			s.MergeTreeMinBytesForConcurrentRead.Must())
	}
	if s.MaxBytesBeforeExternalGroupBy.Valid && s.MaxBytesBeforeExternalGroupBy.Must() < 0 {
		return semerr.InvalidInputf("invalid max_bytes_before_external_group_by value %d: must be non-negative",
			s.MaxBytesBeforeExternalGroupBy.Must())
	}
	if s.MaxBytesBeforeExternalSort.Valid && s.MaxBytesBeforeExternalSort.Must() < 0 {
		return semerr.InvalidInputf("invalid max_bytes_before_external_sort value %d: must be non-negative",
			s.MaxBytesBeforeExternalSort.Must())
	}
	if s.GroupByTwoLevelThresholdBytes.Valid && s.GroupByTwoLevelThresholdBytes.Must() < 0 {
		return semerr.InvalidInputf("invalid group_by_two_level_threshold_bytes value %d: must be non-negative",
			s.GroupByTwoLevelThresholdBytes.Must())
	}

	// Resource usage limits and query priorities
	if s.MaxThreads.Valid && s.MaxThreads.Must() < 1 {
		return semerr.InvalidInputf("invalid max_threads value %d: must be positive",
			s.MaxThreads.Must())
	}
	if s.MaxMemoryUsage.Valid && s.MaxMemoryUsage.Must() < 0 {
		return semerr.InvalidInputf("invalid max_memory_usage value %d: must be non-negative",
			s.MaxMemoryUsage.Must())
	}
	if s.MaxMemoryUsageForUser.Valid && s.MaxMemoryUsageForUser.Must() < 0 {
		return semerr.InvalidInputf("invalid max_memory_usage_for_user value %d: must be non-negative",
			s.MaxMemoryUsageForUser.Must())
	}
	if s.MaxNetworkBandwidth.Valid && s.MaxNetworkBandwidth.Must() < 0 {
		return semerr.InvalidInputf("invalid max_network_bandwidth value %d: must be non-negative",
			s.MaxNetworkBandwidth.Must())
	}
	if s.MaxNetworkBandwidthForUser.Valid && s.MaxNetworkBandwidthForUser.Must() < 0 {
		return semerr.InvalidInputf("invalid max_network_bandwidth_for_user value %d: must be non-negative",
			s.MaxNetworkBandwidthForUser.Must())
	}

	// Query complexity limits
	if s.MaxRowsToRead.Valid && s.MaxRowsToRead.Must() < 0 {
		return semerr.InvalidInputf("invalid max_rows_to_read value %d: must be non-negative",
			s.MaxRowsToRead.Must())
	}
	if s.MaxBytesToRead.Valid && s.MaxBytesToRead.Must() < 0 {
		return semerr.InvalidInputf("invalid max_bytes_to_read value %d: must be non-negative",
			s.MaxBytesToRead.Must())
	}
	if s.MaxRowsToGroupBy.Valid && s.MaxRowsToGroupBy.Must() < 0 {
		return semerr.InvalidInputf("invalid max_rows_to_group_by value %d: must be non-negative",
			s.MaxRowsToGroupBy.Must())
	}
	if s.MaxRowsToSort.Valid && s.MaxRowsToSort.Must() < 0 {
		return semerr.InvalidInputf("invalid max_rows_to_sort value %d: must be non-negative",
			s.MaxRowsToSort.Must())
	}
	if s.MaxBytesToSort.Valid && s.MaxBytesToSort.Must() < 0 {
		return semerr.InvalidInputf("invalid max_bytes_to_sort value %d: must be non-negative",
			s.MaxBytesToSort.Must())
	}
	if s.MaxResultRows.Valid && s.MaxResultRows.Must() < 0 {
		return semerr.InvalidInputf("invalid max_result_rows value %d: must be non-negative",
			s.MaxResultRows.Must())
	}
	if s.MaxResultBytes.Valid && s.MaxResultBytes.Must() < 0 {
		return semerr.InvalidInputf("invalid max_result_bytes value %d: must be non-negative",
			s.MaxResultBytes.Must())
	}
	if s.MaxRowsInDistinct.Valid && s.MaxRowsInDistinct.Must() < 0 {
		return semerr.InvalidInputf("invalid max_rows_in_distinct value %d: must be non-negative",
			s.MaxRowsInDistinct.Must())
	}
	if s.MaxBytesInDistinct.Valid && s.MaxBytesInDistinct.Must() < 0 {
		return semerr.InvalidInputf("invalid max_bytes_in_distinct value %d: must be non-negative",
			s.MaxBytesInDistinct.Must())
	}
	if s.MaxRowsToTransfer.Valid && s.MaxRowsToTransfer.Must() < 0 {
		return semerr.InvalidInputf("invalid max_rows_to_transfer value %d: must be non-negative",
			s.MaxRowsToTransfer.Must())
	}
	if s.MaxBytesToTransfer.Valid && s.MaxBytesToTransfer.Must() < 0 {
		return semerr.InvalidInputf("invalid max_bytes_to_transfer value %d: must be non-negative",
			s.MaxBytesToTransfer.Must())
	}
	if s.MaxExecutionTime.Valid && s.MaxExecutionTime.Must() < 0 {
		return semerr.InvalidInputf(
			"invalid max_execution_time value %d: must be non-negative", s.MaxExecutionTime.Must())
	}
	if s.MaxRowsInSet.Valid && s.MaxRowsInSet.Must() < 0 {
		return semerr.InvalidInputf("invalid max_rows_in_set value %d: must be non-negative",
			s.MaxRowsInSet.Must())
	}
	if s.MaxBytesInSet.Valid && s.MaxBytesInSet.Must() < 0 {
		return semerr.InvalidInputf("invalid max_bytes_in_set value %d: must be non-negative",
			s.MaxBytesInSet.Must())
	}
	if s.MaxRowsInJoin.Valid && s.MaxRowsInJoin.Must() < 0 {
		return semerr.InvalidInputf("invalid max_rows_in_join value %d: must be non-negative",
			s.MaxRowsInJoin.Must())
	}
	if s.MaxBytesInJoin.Valid && s.MaxBytesInJoin.Must() < 0 {
		return semerr.InvalidInputf("invalid max_bytes_in_join value %d: must be non-negative",
			s.MaxBytesInJoin.Must())
	}
	if s.MaxColumnsToRead.Valid && s.MaxColumnsToRead.Must() < 0 {
		return semerr.InvalidInputf("invalid max_columns_to_read value %d: must be non-negative",
			s.MaxColumnsToRead.Must())
	}
	if s.MaxTemporaryColumns.Valid && s.MaxTemporaryColumns.Must() < 0 {
		return semerr.InvalidInputf("invalid max_temporary_columns value %d: must be non-negative",
			s.MaxTemporaryColumns.Must())
	}
	if s.MaxTemporaryNonConstColumns.Valid && s.MaxTemporaryNonConstColumns.Must() < 0 {
		return semerr.InvalidInputf("invalid max_temporary_non_const_columns value %d: must be non-negative",
			s.MaxTemporaryNonConstColumns.Must())
	}
	if s.MaxQuerySize.Valid && s.MaxQuerySize.Must() < 0 {
		return semerr.InvalidInputf("invalid max_query_size value %d: must be non-negative",
			s.MaxQuerySize.Must())
	}
	if s.MaxAstDepth.Valid && s.MaxAstDepth.Must() < 0 {
		return semerr.InvalidInputf("invalid max_ast_depth value %d: must be non-negative",
			s.MaxAstDepth.Must())
	}
	if s.MaxAstElements.Valid && s.MaxAstElements.Must() < 0 {
		return semerr.InvalidInputf("invalid max_ast_elements value %d: must be non-negative",
			s.MaxAstElements.Must())
	}
	if s.MaxExpandedAstElements.Valid && s.MaxExpandedAstElements.Must() < 0 {
		return semerr.InvalidInputf("invalid max_expanded_ast_elements value %d: must be non-negative",
			s.MaxExpandedAstElements.Must())
	}
	if s.MinExecutionSpeed.Valid && s.MinExecutionSpeed.Must() < 0 {
		return semerr.InvalidInputf("invalid min_execution_speed value %d: must be non-negative",
			s.MinExecutionSpeed.Must())
	}
	if s.MinExecutionSpeedBytes.Valid && s.MinExecutionSpeedBytes.Must() < 0 {
		return semerr.InvalidInputf("invalid min_execution_speed_bytes value %d: must be non-negative",
			s.MinExecutionSpeedBytes.Must())
	}

	// HTTP-specific settings
	if s.HTTPConnectionTimeout.Valid && s.HTTPConnectionTimeout.Must() < 1 {
		return semerr.InvalidInput(
			"invalid http_connection_timeout value: must be at least 1000 (1 second)")
	}
	if s.HTTPReceiveTimeout.Valid && s.HTTPReceiveTimeout.Must() < 1 {
		return semerr.InvalidInput(
			"invalid http_receive_timeout value: must be at least 1000 (1 second)")
	}
	if s.HTTPSendTimeout.Valid && s.HTTPSendTimeout.Must() < 1 {
		return semerr.InvalidInput(
			"invalid http_receive_timeout value: must be at least 1000 (1 second)")
	}
	if s.HTTPHeadersProgressInterval.Valid && s.HTTPHeadersProgressInterval.Must() < 100 {
		return semerr.InvalidInputf(
			"invalid http_headers_progress_interval value %d: must be 100 or greater",
			s.HTTPHeadersProgressInterval.Must())
	}

	return nil
}

var userNameValidator = models.MustUserNameValidator("^[a-zA-Z_][a-zA-Z0-9_-]*$", []string{"default"})
var userPasswordValidator = models.MustUserPasswordValidator(models.DefaultUserPasswordPattern)

func validateUserPassword(password secret.String) error {
	return userPasswordValidator.ValidateString(password.Unmask())
}
