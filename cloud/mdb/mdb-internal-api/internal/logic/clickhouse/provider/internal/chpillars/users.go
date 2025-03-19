package chpillars

import (
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
)

type User struct {
	Hash      pillars.CryptoKey   `json:"hash"`
	Password  pillars.CryptoKey   `json:"password"`
	Databases map[string]Database `json:"databases"`
	Settings  UserSettings        `json:"settings"`
	Quotas    []UserQuota         `json:"quotas"`
}

type Table struct {
	Filter string `json:"filter,omitempty"`
}

type Database struct {
	Tables map[string]Table `json:"tables,omitempty"`
}

type UserSettings struct {
	// Permissions
	Readonly *int64 `name:"readonly" json:"readonly,omitempty"`
	AllowDDL *int64 `name:"allow_ddl" json:"allow_ddl,omitempty"`

	// Timeouts
	ConnectTimeout *int64 `name:"connect_timeout" json:"connect_timeout,omitempty"`
	ReceiveTimeout *int64 `name:"receive_timeout" json:"receive_timeout,omitempty"`
	SendTimeout    *int64 `name:"send_timeout" json:"send_timeout,omitempty"`

	// Replication settings
	InsertQuorum                   *int64 `name:"insert_quorum" json:"insert_quorum,omitempty"`
	InsertQuorumTimeout            *int64 `name:"insert_quorum_timeout" json:"insert_quorum_timeout,omitempty"`
	InsertQuorumParallel           *int64 `name:"insert_quorum_parallel" json:"insert_quorum_parallel,omitempty"`
	SelectSequentialConsistency    *int64 `name:"select_sequential_consistency" json:"select_sequential_consistency,omitempty"`
	ReplicationAlterPartitionsSync *int64 `name:"replication_alter_partitions_sync" json:"replication_alter_partitions_sync,omitempty"`

	// Settings of distributed queries
	MaxReplicaDelayForDistributedQueries         *int64                          `name:"max_replica_delay_for_distributed_queries" json:"max_replica_delay_for_distributed_queries,omitempty"`
	FallbackToStaleReplicasForDistributedQueries *int64                          `name:"fallback_to_stale_replicas_for_distributed_queries" json:"fallback_to_stale_replicas_for_distributed_queries,omitempty"`
	DistributedProductMode                       chmodels.DistributedProductMode `name:"distributed_product_mode" json:"distributed_product_mode,omitempty"`
	DistributedAggregationMemoryEfficient        *int64                          `name:"distributed_aggregation_memory_efficient" json:"distributed_aggregation_memory_efficient,omitempty"`
	DistributedDDLTaskTimeout                    *int64                          `name:"distributed_ddl_task_timeout" json:"distributed_ddl_task_timeout,omitempty"`
	SkipUnavailableShards                        *int64                          `name:"skip_unavailable_shards" json:"skip_unavailable_shards,omitempty"`
	LoadBalancing                                chmodels.LoadBalancing          `name:"load_balancing" json:"load_balancing,omitempty"`

	// Query and expression compilations
	Compile                     *int64 `name:"compile" json:"compile,omitempty"`
	MinCountToCompile           *int64 `name:"min_count_to_compile" json:"min_count_to_compile,omitempty"`
	CompileExpressions          *int64 `name:"compile_expressions" json:"compile_expressions,omitempty"`
	MinCountToCompileExpression *int64 `name:"min_count_to_compile_expression" json:"min_count_to_compile_expression,omitempty"`
	OptimizeMoveToPrewhere      *int64 `name:"optimize_move_to_prewhere" json:"optimize_move_to_prewhere,omitempty"`

	// I/O settings
	MaxBlockSize                                  *int64 `name:"max_block_size" json:"max_block_size,omitempty"`
	MinInsertBlockSizeRows                        *int64 `name:"min_insert_block_size_rows" json:"min_insert_block_size_rows,omitempty"`
	MinInsertBlockSizeBytes                       *int64 `name:"min_insert_block_size_bytes" json:"min_insert_block_size_bytes,omitempty"`
	MaxInsertBlockSize                            *int64 `name:"max_insert_block_size" json:"max_insert_block_size,omitempty"`
	MaxPartitionsPerInsertBlock                   *int64 `name:"max_partitions_per_insert_block" json:"max_partitions_per_insert_block,omitempty"`
	MinBytesToUseDirectIO                         *int64 `name:"min_bytes_to_use_direct_io" json:"min_bytes_to_use_direct_io,omitempty"`
	UseUncompressedCache                          *int64 `name:"use_uncompressed_cache" json:"use_uncompressed_cache,omitempty"`
	MergeTreeMaxRowsToUseCache                    *int64 `name:"merge_tree_max_rows_to_use_cache" json:"merge_tree_max_rows_to_use_cache,omitempty"`
	MergeTreeMaxBytesToUseCache                   *int64 `name:"merge_tree_max_bytes_to_use_cache" json:"merge_tree_max_bytes_to_use_cache,omitempty"`
	MergeTreeMinRowsForConcurrentRead             *int64 `name:"merge_tree_min_rows_for_concurrent_read" json:"merge_tree_min_rows_for_concurrent_read,omitempty"`
	MergeTreeMinBytesForConcurrentRead            *int64 `name:"merge_tree_min_bytes_for_concurrent_read" json:"merge_tree_min_bytes_for_concurrent_read,omitempty"`
	MaxBytesBeforeExternalGroupBy                 *int64 `name:"max_bytes_before_external_group_by" json:"max_bytes_before_external_group_by,omitempty"`
	MaxBytesBeforeExternalSort                    *int64 `name:"max_bytes_before_external_sort" json:"max_bytes_before_external_sort,omitempty"`
	GroupByTwoLevelThreshold                      *int64 `name:"group_by_two_level_threshold" json:"group_by_two_level_threshold,omitempty"`
	GroupByTwoLevelThresholdBytes                 *int64 `name:"group_by_two_level_threshold_bytes" json:"group_by_two_level_threshold_bytes,omitempty"`
	DeduplicateBlocksInDependentMaterializedViews *int64 `name:"deduplicate_blocks_in_dependent_materialized_views" json:"deduplicate_blocks_in_dependent_materialized_views,omitempty"`

	// Resource usage limits and query priorities
	Priority                    *int64 `name:"priority" json:"priority,omitempty"`
	MaxThreads                  *int64 `name:"max_threads" json:"max_threads,omitempty"`
	MaxMemoryUsage              *int64 `name:"max_memory_usage" json:"max_memory_usage,omitempty"`
	MaxMemoryUsageForUser       *int64 `name:"max_memory_usage_for_user" json:"max_memory_usage_for_user,omitempty"`
	MaxNetworkBandwidth         *int64 `name:"max_network_bandwidth" json:"max_network_bandwidth,omitempty"`
	MaxNetworkBandwidthForUser  *int64 `name:"max_network_bandwidth_for_user" json:"max_network_bandwidth_for_user,omitempty"`
	MaxConcurrentQueriesForUser *int64 `name:"max_concurrent_queries_for_user" json:"max_concurrent_queries_for_user,omitempty"`

	// Query complexity limits
	ForceIndexByDate            *int64                       `name:"force_index_by_date" json:"force_index_by_date,omitempty"`
	ForcePrimaryKey             *int64                       `name:"force_primary_key" json:"force_primary_key,omitempty"`
	MaxRowsToRead               *int64                       `name:"max_rows_to_read" json:"max_rows_to_read,omitempty"`
	MaxBytesToRead              *int64                       `name:"max_bytes_to_read" json:"max_bytes_to_read,omitempty"`
	ReadOverflowMode            chmodels.OverflowMode        `name:"read_overflow_mode" json:"read_overflow_mode,omitempty"`
	MaxRowsToGroupBy            *int64                       `name:"max_rows_to_group_by" json:"max_rows_to_group_by,omitempty"`
	GroupByOverflowMode         chmodels.GroupByOverflowMode `name:"group_by_overflow_mode" json:"group_by_overflow_mode,omitempty"`
	MaxRowsToSort               *int64                       `name:"max_rows_to_sort" json:"max_rows_to_sort,omitempty"`
	MaxBytesToSort              *int64                       `name:"max_bytes_to_sort" json:"max_bytes_to_sort,omitempty"`
	SortOverflowMode            chmodels.OverflowMode        `name:"sort_overflow_mode" json:"sort_overflow_mode,omitempty"`
	MaxResultRows               *int64                       `name:"max_result_rows" json:"max_result_rows,omitempty"`
	MaxResultBytes              *int64                       `name:"max_result_bytes" json:"max_result_bytes,omitempty"`
	ResultOverflowMode          chmodels.OverflowMode        `name:"result_overflow_mode" json:"result_overflow_mode,omitempty"`
	MaxRowsInDistinct           *int64                       `name:"max_rows_in_distinct" json:"max_rows_in_distinct,omitempty"`
	MaxBytesInDistinct          *int64                       `name:"max_bytes_in_distinct" json:"max_bytes_in_distinct,omitempty"`
	DistinctOverflowMode        chmodels.OverflowMode        `name:"distinct_overflow_mode" json:"distinct_overflow_mode,omitempty"`
	MaxRowsToTransfer           *int64                       `name:"max_rows_to_transfer" json:"max_rows_to_transfer,omitempty"`
	MaxBytesToTransfer          *int64                       `name:"max_bytes_to_transfer" json:"max_bytes_to_transfer,omitempty"`
	TransferOverflowMode        chmodels.OverflowMode        `name:"transfer_overflow_mode" json:"transfer_overflow_mode,omitempty"`
	MaxExecutionTime            *int64                       `name:"max_execution_time" json:"max_execution_time,omitempty"`
	TimeoutOverflowMode         chmodels.OverflowMode        `name:"timeout_overflow_mode" json:"timeout_overflow_mode,omitempty"`
	MaxRowsInSet                *int64                       `name:"max_rows_in_set" json:"max_rows_in_set,omitempty"`
	MaxBytesInSet               *int64                       `name:"max_bytes_in_set" json:"max_bytes_in_set,omitempty"`
	SetOverflowMode             chmodels.OverflowMode        `name:"set_overflow_mode" json:"set_overflow_mode,omitempty"`
	MaxRowsInJoin               *int64                       `name:"max_rows_in_join" json:"max_rows_in_join,omitempty"`
	MaxBytesInJoin              *int64                       `name:"max_bytes_in_join" json:"max_bytes_in_join,omitempty"`
	JoinOverflowMode            chmodels.OverflowMode        `name:"join_overflow_mode" json:"join_overflow_mode,omitempty"`
	MaxColumnsToRead            *int64                       `name:"max_columns_to_read" json:"max_columns_to_read,omitempty"`
	MaxTemporaryColumns         *int64                       `name:"max_temporary_columns" json:"max_temporary_columns,omitempty"`
	MaxTemporaryNonConstColumns *int64                       `name:"max_temporary_non_const_columns" json:"max_temporary_non_const_columns,omitempty"`
	MaxQuerySize                *int64                       `name:"max_query_size" json:"max_query_size,omitempty"`
	MaxAstDepth                 *int64                       `name:"max_ast_depth" json:"max_ast_depth,omitempty"`
	MaxAstElements              *int64                       `name:"max_ast_elements" json:"max_ast_elements,omitempty"`
	MaxExpandedAstElements      *int64                       `name:"max_expanded_ast_elements" json:"max_expanded_ast_elements,omitempty"`
	MinExecutionSpeed           *int64                       `name:"min_execution_speed" json:"min_execution_speed,omitempty"`
	MinExecutionSpeedBytes      *int64                       `name:"min_execution_speed_bytes" json:"min_execution_speed_bytes,omitempty"`

	// Settings of input and output formats
	InputFormatValuesInterpretExpressions *int64                            `name:"input_format_values_interpret_expressions" json:"input_format_values_interpret_expressions,omitempty"`
	InputFormatDefaultsForOmittedFields   *int64                            `name:"input_format_defaults_for_omitted_fields" json:"input_format_defaults_for_omitted_fields,omitempty"`
	InputFormatWithNamesUseHeader         *int64                            `name:"input_format_with_names_use_header" json:"input_format_with_names_use_header,omitempty"`
	InputFormatNullAsDefault              *int64                            `name:"input_format_null_as_default" json:"input_format_null_as_default,omitempty"`
	OutputFormatJSONQuote64bitIntegers    *int64                            `name:"output_format_json_quote_" json:"output_format_json_quote_64bit_integers,omitempty"`
	OutputFormatJSONQuoteDenormals        *int64                            `name:"output_format_json_quote_denormals" json:"output_format_json_quote_denormals,omitempty"`
	LowCardinalityAllowInNativeFormat     *int64                            `name:"low_cardinality_allow_in_native_format" json:"low_cardinality_allow_in_native_format,omitempty"`
	EmptyResultForAggregationByEmptySet   *int64                            `name:"empty_result_for_aggregation_by_empty_set" json:"empty_result_for_aggregation_by_empty_set,omitempty"`
	DateTimeInputFormat                   chmodels.DateTimeInputFormat      `name:"date_time_input_format" json:"date_time_input_format,omitempty"`
	DateTimeOutputFormat                  chmodels.DateTimeOutputFormat     `name:"date_time_output_format" json:"date_time_output_format,omitempty"`
	FormatRegexp                          string                            `name:"format_regexp" json:"format_regexp,omitempty"`
	FormatRegexpEscapingRule              chmodels.FormatRegexpEscapingRule `name:"format_regexp_escaping_rule" json:"format_regexp_escaping_rule,omitempty"`
	FormatRegexpSkipUnmatched             *int64                            `name:"format_regexp_skip_unmatched" json:"format_regexp_skip_unmatched,omitempty"`

	// HTTP-specific settings
	HTTPConnectionTimeout       *int64 `name:"http_connection_timeout" json:"http_connection_timeout,omitempty"`
	HTTPReceiveTimeout          *int64 `name:"http_receive_timeout" json:"http_receive_timeout,omitempty"`
	HTTPSendTimeout             *int64 `name:"http_send_timeout" json:"http_send_timeout,omitempty"`
	EnableHTTPCompression       *int64 `name:"enable_http_compression" json:"enable_http_compression,omitempty"`
	SendProgressInHTTPHeaders   *int64 `name:"send_progress_in_http_headers" json:"send_progress_in_http_headers,omitempty"`
	HTTPHeadersProgressInterval *int64 `name:"http_headers_progress_interval_ms" json:"http_headers_progress_interval_ms,omitempty"`
	AddHTTPCorsHeader           *int64 `name:"add_http_cors_header" json:"add_http_cors_header,omitempty"`

	// Quoting settings
	QuotaMode chmodels.QuotaMode `name:"quota_mode" json:"quota_mode,omitempty"`

	// Other settings
	CountDistinctImplementation   chmodels.CountDistinctImplementation `name:"count_distinct_implementation" json:"count_distinct_implementation,omitempty"`
	JoinAlgorithm                 chmodels.JoinAlgorithm               `name:"join_algorithm" json:"join_algorithm,omitempty"`
	AnyJoinDistinctRightTableKeys *int64                               `name:"any_join_distinct_right_table_keys" json:"any_join_distinct_right_table_keys,omitempty"`
	JoinedSubqueryRequiresAlias   *int64                               `name:"joined_subquery_requires_alias" json:"joined_subquery_requires_alias,omitempty"`
	JoinUseNulls                  *int64                               `name:"join_use_nulls" json:"join_use_nulls,omitempty"`
	TransformNullIn               *int64                               `name:"transform_null_in" json:"transform_null_in,omitempty"`
}

type UserQuota struct {
	IntervalDuration int64  `name:"interval_duration" json:"interval_duration"`
	Queries          *int64 `name:"queries" json:"queries"`
	Errors           *int64 `name:"errors" json:"errors"`
	ResultRows       *int64 `name:"result_rows" json:"result_rows"`
	ReadRows         *int64 `name:"read_rows" json:"read_rows"`
	ExecutionTime    *int64 `name:"execution_time" json:"execution_time"`
}

func (p *SubClusterCH) AddUser(us chmodels.UserSpec, password, passwordHash pillars.CryptoKey) error {
	// Add default permissions
	if len(us.Permissions) == 0 {
		for _, dbName := range p.Data.ClickHouse.DBs {
			us.Permissions = append(us.Permissions, chmodels.Permission{
				DatabaseName: dbName,
				DataFilters:  nil,
			})
		}
	}

	if _, ok := p.Data.ClickHouse.Users[us.Name]; ok {
		return semerr.AlreadyExistsf("user %q already exists", us.Name)
	}

	if err := p.validatePermissions(us.Permissions); err != nil {
		return err
	}

	settings, err := FormatSettings(us.Settings)
	if err != nil {
		return err
	}

	user := User{
		Hash:      passwordHash,
		Password:  password,
		Databases: FormatPermissions(us.Permissions),
		Settings:  settings,
		Quotas:    FormatUserQuotas(us.UserQuotas),
	}

	p.Data.ClickHouse.Users[us.Name] = user
	return nil
}

func (p *SubClusterCH) DeleteUser(username string) error {
	if _, ok := p.Data.ClickHouse.Users[username]; !ok {
		return semerr.NotFoundf("user %q not found", username)
	}

	delete(p.Data.ClickHouse.Users, username)
	return nil
}

func (p *SubClusterCH) UpdateUserPassword(username string, password, hash pillars.CryptoKey) error {
	user, ok := p.Data.ClickHouse.Users[username]
	if !ok {
		return semerr.NotFoundf("user %q not found", username)
	}

	user.Password = password
	user.Hash = hash
	p.Data.ClickHouse.Users[username] = user

	return nil
}

func (p *SubClusterCH) ModifyUser(username string, spec chmodels.UpdateUserArgs) error {
	user, ok := p.Data.ClickHouse.Users[username]
	if !ok {
		return semerr.NotFoundf("user %q not found", username)
	}

	if spec.UserQuotas != nil {
		user.Quotas = FormatUserQuotas(*spec.UserQuotas)
	}

	if spec.Permissions != nil {
		if err := p.validatePermissions(*spec.Permissions); err != nil {
			return err
		}

		user.Databases = FormatPermissions(*spec.Permissions)
	}

	if spec.Settings != nil {
		if err := updateUserSettings(&user, spec); err != nil {
			return err
		}
	}

	p.Data.ClickHouse.Users[username] = user

	return nil
}

func (p *SubClusterCH) GrantPermission(username string, perm chmodels.Permission) error {
	user, ok := p.Data.ClickHouse.Users[username]
	if !ok {
		return semerr.NotFoundf("user %q not found", username)
	}

	if err := p.validateDatabase(perm.DatabaseName); err != nil {
		return err
	}

	if _, ok := user.Databases[perm.DatabaseName]; ok {
		return semerr.FailedPreconditionf("user %q already has access to database %q", username, perm.DatabaseName)
	}

	var db = Database{Tables: make(map[string]Table)}
	for _, table := range perm.DataFilters {
		db.Tables[table.TableName] = Table{Filter: table.Filter}
	}

	p.Data.ClickHouse.Users[username].Databases[perm.DatabaseName] = db
	return nil
}

func (p *SubClusterCH) RevokePermission(username, database string) error {
	user, ok := p.Data.ClickHouse.Users[username]
	if !ok {
		return semerr.NotFoundf("user %q not found", username)
	}

	if err := p.validateDatabase(database); err != nil {
		return err
	}

	if _, ok := user.Databases[database]; !ok {
		return semerr.FailedPreconditionf("user %q has no access to the database %q", username, database)
	}

	delete(p.Data.ClickHouse.Users[username].Databases, database)
	return nil

}

func (p *SubClusterCH) validatePermissions(permissions []chmodels.Permission) error {
	for _, perm := range permissions {
		if err := p.validateDatabase(perm.DatabaseName); err != nil {
			return err
		}
	}

	return nil
}

func (p *SubClusterCH) validateDatabase(dbname string) error {
	for _, db := range p.Data.ClickHouse.DBs {
		if db == dbname {
			return nil
		}
	}

	return semerr.NotFoundf("database %q not found", dbname)
}

func updateUserSettings(u *User, spec chmodels.UpdateUserArgs) error {
	settings := UserSettings{}
	if err := pillars.MapModelToPillar(spec.Settings, &settings); err != nil {
		return err
	}

	return pillars.UpdatePillarByFieldMask(&u.Settings, &settings, spec.SettingsMask)
}

func UserFromPillar(cid string, userName string, user User) (chmodels.User, error) {
	settings, err := UserSettingsFromPillar(user.Settings)
	if err != nil {
		return chmodels.User{}, err
	}

	res := chmodels.User{
		ClusterID:   cid,
		Name:        userName,
		Permissions: UserPermissionsFromPillar(user.Databases),
		Settings:    settings,
		UserQuotas:  UserQuotasFromPillar(user.Quotas),
	}

	return res, nil
}

func FormatPermissions(permissions []chmodels.Permission) map[string]Database {
	props := make(map[string]Database)
	for _, perm := range permissions {
		var db Database
		if perm.DataFilters != nil {
			db.Tables = make(map[string]Table)
			for _, flt := range perm.DataFilters {
				db.Tables[flt.TableName] = Table{Filter: flt.Filter}
			}
		}
		props[perm.DatabaseName] = db
	}
	return props
}

func UserPermissionsFromPillar(databases map[string]Database) []chmodels.Permission {
	permissions := make([]chmodels.Permission, 0, len(databases))
	for dbName, dbProps := range databases {
		permission := chmodels.Permission{DatabaseName: dbName}
		if dbProps.Tables != nil {
			permission.DataFilters = make([]chmodels.DataFilter, 0, len(dbProps.Tables))
			for tableName, tableProp := range dbProps.Tables {
				permission.DataFilters = append(permission.DataFilters, chmodels.DataFilter{
					TableName: tableName,
					Filter:    tableProp.Filter,
				})
			}
		}
		permissions = append(permissions, permission)
	}
	return permissions
}

func FormatSettings(settings chmodels.UserSettings) (UserSettings, error) {
	pillarSettings := UserSettings{}
	if err := pillars.MapModelToPillar(&settings, &pillarSettings); err != nil {
		return UserSettings{}, err
	}
	return pillarSettings, nil
}

func UserSettingsFromPillar(settings UserSettings) (chmodels.UserSettings, error) {
	res := chmodels.UserSettings{}
	if err := pillars.MapPillarToModel(&settings, &res); err != nil {
		return chmodels.UserSettings{}, err
	}

	return res, nil
}

func UserQuotasFromPillar(quotas []UserQuota) []chmodels.UserQuota {
	var result []chmodels.UserQuota
	for _, quota := range quotas {
		q := chmodels.UserQuota{
			IntervalDuration: pillars.MapPtrSecondsToOptionalDuration(&quota.IntervalDuration).Duration,
			Queries:          pillars.MapPtrInt64ToOptionalInt64(quota.Queries),
			Errors:           pillars.MapPtrInt64ToOptionalInt64(quota.Errors),
			ResultRows:       pillars.MapPtrInt64ToOptionalInt64(quota.ResultRows),
			ReadRows:         pillars.MapPtrInt64ToOptionalInt64(quota.ReadRows),
			ExecutionTime:    pillars.MapPtrSecondsToOptionalDuration(quota.ExecutionTime),
		}

		result = append(result, q)
	}

	return result
}

func FormatUserQuotas(quotas []chmodels.UserQuota) []UserQuota {
	result := make([]UserQuota, len(quotas))
	for i, quota := range quotas {
		result[i] = UserQuota{
			IntervalDuration: int64(quota.IntervalDuration.Seconds()),
			Queries:          pillars.MapOptionalInt64ToPtrInt64(quota.Queries),
			Errors:           pillars.MapOptionalInt64ToPtrInt64(quota.Errors),
			ResultRows:       pillars.MapOptionalInt64ToPtrInt64(quota.ResultRows),
			ReadRows:         pillars.MapOptionalInt64ToPtrInt64(quota.ReadRows),
			ExecutionTime:    pillars.MapOptionalDurationToPtrSeconds(quota.ExecutionTime),
		}
	}

	return result
}
