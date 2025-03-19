package gpmodels

import (
	"strconv"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
)

var (
	mapLogStatementToPillar = map[LogStatement]string{
		LogStatementUnspecified: "",
		LogStatementNone:        "none",
		LogStatementDDL:         "ddl",
		LogStatementMod:         "mod",
		LogStatementAll:         "all",
	}
	mapLogStatementFromPillar = reflectutil.ReverseMap(mapLogStatementToPillar).(map[string]LogStatement)
)

func (c *ClusterConfig) FromPillar() ConfigBase {
	res := ConfigBase{}

	if c.MaxConnections != nil && c.MaxConnections.Master != nil {
		res.MaxConnections.Set(*c.MaxConnections.Master)
	}
	if c.GPWorkfileLimitFilesPerQuery != nil {
		res.GPWorkfileLimitFilesPerQuery.Set(*c.GPWorkfileLimitFilesPerQuery)
	}
	if c.GPWorkfileCompression != nil {
		res.GPWorkfileCompression.Set(*c.GPWorkfileCompression)
	}
	if c.GPWorkfileLimitPerQuery != nil {
		res.GPWorkfileLimitPerQuery.Set(PGSizeTOBytes(*c.GPWorkfileLimitPerQuery))
	}
	if c.MaxSlotWalKeepSize != nil {
		res.MaxSlotWalKeepSize.Set(PGSizeTOBytes(*c.MaxSlotWalKeepSize))
	}
	if c.MaxPreparedTransactions != nil {
		res.MaxPreparedTransactions.Set(*c.MaxPreparedTransactions)
	}
	if c.GPWorkfileLimitPerSegment != nil {
		res.GPWorkfileLimitPerSegment.Set(PGSizeTOBytes(*c.GPWorkfileLimitPerSegment))
	}
	if c.MaxStatementMem != nil {
		res.MaxStatementMem.Set(PGSizeTOBytes(*c.MaxStatementMem))
	}
	if c.LogStatement != nil && c.LogStatement.Master != nil {
		res.LogStatement = LogStatementUnspecified
		ls, ok := mapLogStatementFromPillar[*c.LogStatement.Master]
		if ok {
			res.LogStatement = ls
		}
	}

	return res
}

func BytesToMB(v int64) *string {
	val := strconv.Itoa(int(v/1024/1024)) + "MB"
	return &val
}

func PGSizeTOBytes(v string) int64 {

	multi := map[string]int64{"KB": 1024, "MB": 1024 * 1024, "GB": 1024 * 1024 * 1024}

	suffix := v[len(v)-2:]
	val, found := multi[suffix]
	if !found {
		return 0
	}
	i, err := strconv.Atoi(v[:len(v)-2])
	if err != nil {
		return 0
	}
	return int64(i) * val
}

func (c *ConfigBase) Validate() error {
	const (
		min = "min"
		max = "max"
	)
	// TODO: increase upper limit for max connections when shared_buffers change becomes possible
	maxConnections := map[string]int64{min: 250, max: 1000}
	maxPreparedTransactions := map[string]int64{min: 350, max: 10000}
	maxSlotWalKeepSize := map[string]int64{min: 0, max: 214748364800}
	gPWorkfileLimitFilesPerQuery := map[string]int64{min: 0, max: 100000}
	gPWorkfileLimitPerQuery := map[string]int64{min: 0, max: 1099511627776}
	gPWorkfileLimitPerSegment := map[string]int64{min: 0, max: 1099511627776}
	maxStatementMem := map[string]int64{min: 128 * 1024 * 1024, max: 1024 * 1024 * 1024 * 1024}

	if c.GPWorkfileLimitPerQuery.Valid {
		if c.GPWorkfileLimitPerQuery.Int64 > gPWorkfileLimitPerQuery[max] {
			return semerr.InvalidInputf("invalid gp_workfile_limit_per_query value %d: must be %d or lower)", c.GPWorkfileLimitPerQuery.Int64, gPWorkfileLimitPerQuery[max])
		}
		if c.GPWorkfileLimitPerQuery.Int64 < gPWorkfileLimitPerQuery[min] {
			return semerr.InvalidInputf("invalid gp_workfile_limit_per_query value %d: must be %d or greater", c.GPWorkfileLimitPerQuery.Int64, gPWorkfileLimitPerQuery[min])
		}
		if c.GPWorkfileLimitPerQuery.Int64%1048576 != 0 {
			return semerr.InvalidInputf("invalid gp_workfile_limit_per_query %d: must be multiple by 1048576 (1MB)", c.GPWorkfileLimitPerQuery.Int64)
		}
	}

	if c.GPWorkfileLimitPerSegment.Valid {
		if c.GPWorkfileLimitPerSegment.Int64 > gPWorkfileLimitPerSegment[max] {
			return semerr.InvalidInputf("invalid gp_workfile_limit_per_segment value %d: must be %d or lower)", c.GPWorkfileLimitPerSegment.Int64, gPWorkfileLimitPerSegment[max])
		}
		if c.GPWorkfileLimitPerSegment.Int64 < gPWorkfileLimitPerSegment[min] {
			return semerr.InvalidInputf("invalid gp_workfile_limit_per_segment value %d: must be %d or greater", c.GPWorkfileLimitPerSegment.Int64, gPWorkfileLimitPerSegment[min])
		}
		if c.GPWorkfileLimitPerSegment.Int64%1048576 != 0 {
			return semerr.InvalidInputf("invalid gp_workfile_limit_per_segment %d: must be multiple by 1048576 (1MB)", c.GPWorkfileLimitPerSegment.Int64)
		}
	}

	if c.MaxConnections.Valid {
		if c.MaxConnections.Int64 <= maxConnections[min] {
			return semerr.InvalidInputf("invalid max_connections value %d: must be %d or greater", c.MaxConnections.Int64, maxConnections[min])
		}
		if c.MaxConnections.Int64 > maxConnections[max] {
			return semerr.InvalidInputf("invalid max_connections value %d: must be %d or lower", c.MaxConnections.Int64, maxConnections[max])
		}
	}

	if c.MaxPreparedTransactions.Valid {
		if c.MaxPreparedTransactions.Int64 <= maxPreparedTransactions[min] {
			return semerr.InvalidInputf("invalid max_prepared_transactions value %d: must be %d or greater", c.MaxPreparedTransactions.Int64, maxPreparedTransactions[min])
		}
		if c.MaxPreparedTransactions.Int64 > maxPreparedTransactions[max] {
			return semerr.InvalidInputf("invalid max_prepared_transactions value %d: must be %d or lower", c.MaxPreparedTransactions.Int64, maxPreparedTransactions[max])
		}
	}

	if c.MaxSlotWalKeepSize.Valid {
		if c.MaxSlotWalKeepSize.Int64 > maxSlotWalKeepSize[max] {
			return semerr.InvalidInputf("invalid max_slot_wal_keep_size %d: must be lower %d (200GB)", c.MaxSlotWalKeepSize.Int64, maxSlotWalKeepSize[max])
		}
		if c.MaxSlotWalKeepSize.Int64%1048576 != 0 {
			return semerr.InvalidInputf("invalid max_slot_wal_keep_size %d: must be multiple by 1048576 (1MB)", c.MaxSlotWalKeepSize.Int64)
		}
	}

	if c.GPWorkfileLimitFilesPerQuery.Valid {
		if c.MaxPreparedTransactions.Int64 <= gPWorkfileLimitFilesPerQuery[min] {
			return semerr.InvalidInputf("invalid max_prepared_transactions value %d: must be %d or greater", c.GPWorkfileLimitFilesPerQuery.Int64, gPWorkfileLimitFilesPerQuery[min])
		}
		if c.MaxPreparedTransactions.Int64 > gPWorkfileLimitFilesPerQuery[max] {
			return semerr.InvalidInputf("invalid max_prepared_transactions value %d: must be %d or lower", c.GPWorkfileLimitFilesPerQuery.Int64, gPWorkfileLimitFilesPerQuery[max])
		}
	}
	if c.MaxStatementMem.Valid {
		if c.MaxStatementMem.Int64 > maxStatementMem[max] {
			return semerr.InvalidInputf("invalid max_statement_mem value %d: must be %d or lower", c.MaxStatementMem.Int64, maxStatementMem[max])
		}
		if c.MaxStatementMem.Int64 < maxStatementMem[min] {
			return semerr.InvalidInputf("invalid max_statement_mem value %d: must be %d or greater", c.MaxStatementMem.Int64, maxStatementMem[min])
		}
		if c.MaxStatementMem.Int64%1048576 != 0 {
			return semerr.InvalidInputf("invalid max_statement_mem value %d: must be multiple by 1048576 (1MB)", c.MaxStatementMem.Int64)
		}
	}
	_, ok := mapLogStatementToPillar[c.LogStatement]
	if !ok {
		return semerr.InvalidInputf("invalid log_statement value %d", c.LogStatement)
	}

	return nil
}

func (c *ConfigBase) ToPillar() ClusterConfig {
	res := ClusterConfig{}
	if c.MaxConnections.Valid {
		res.MaxConnections = &IntMasterSegment{}
		res.MaxConnections.Master = &c.MaxConnections.Int64
	}
	if c.MaxSlotWalKeepSize.Valid {
		res.MaxSlotWalKeepSize = BytesToMB(c.MaxSlotWalKeepSize.Int64)
	}
	if c.GPWorkfileLimitPerQuery.Valid {
		res.GPWorkfileLimitPerQuery = BytesToMB(c.GPWorkfileLimitPerQuery.Int64)
	}
	if c.GPWorkfileLimitPerSegment.Valid {
		res.GPWorkfileLimitPerSegment = BytesToMB(c.GPWorkfileLimitPerSegment.Int64)
	}
	if c.GPWorkfileLimitFilesPerQuery.Valid {
		res.GPWorkfileLimitFilesPerQuery = &c.GPWorkfileLimitFilesPerQuery.Int64
	}
	if c.MaxPreparedTransactions.Valid {
		res.MaxPreparedTransactions = &c.MaxPreparedTransactions.Int64
	}
	if c.GPWorkfileCompression.Valid {
		res.GPWorkfileCompression = &c.GPWorkfileCompression.Bool
	}
	if c.MaxStatementMem.Valid {
		res.MaxStatementMem = BytesToMB(c.MaxStatementMem.Int64)
	}
	res.LogStatement = &StringMasterSegment{}
	if c.LogStatement != LogStatementUnspecified {
		lsText := mapLogStatementToPillar[c.LogStatement]
		res.LogStatement.Master = &lsText
	}
	return res
}

func (c *PoolerConfigBase) ToPillar() *PoolerConfig {
	res := PoolerConfig{}
	if c.Mode.Valid {
		res.Mode = &c.Mode.String
	}

	if c.PoolSize.Valid {
		res.PoolSize = &c.PoolSize.Int64
	}

	if c.ClientIdleTimeout.Valid {
		res.ClientIdleTimeout = &c.ClientIdleTimeout.Int64
	}

	return &res
}

type LogStatement int

const (
	LogStatementUnspecified LogStatement = iota
	LogStatementNone
	LogStatementDDL
	LogStatementMod
	LogStatementAll
)

type ConfigBase struct {
	Version                      string
	MaxConnections               optional.Int64
	MaxSlotWalKeepSize           optional.Int64
	GPWorkfileLimitPerSegment    optional.Int64
	GPWorkfileLimitPerQuery      optional.Int64
	GPWorkfileLimitFilesPerQuery optional.Int64
	MaxPreparedTransactions      optional.Int64
	GPWorkfileCompression        optional.Bool
	MaxStatementMem              optional.Int64
	LogStatement                 LogStatement
}

type PoolerConfigBase struct {
	Mode              optional.String
	PoolSize          optional.Int64
	ClientIdleTimeout optional.Int64
}

type PoolerConfig struct {
	Mode              *string `json:"mode,omitempty"`
	PoolSize          *int64  `json:"pool_size,omitempty"`
	ClientIdleTimeout *int64  `json:"client_idle_timeout,omitempty"`
}

type IntMasterSegment struct {
	Master  *int64 `json:"master,omitempty"`
	Segment *int64 `json:"segment,omitempty"`
}
type StringMasterSegment struct {
	Master  *string `json:"master,omitempty"`
	Segment *string `json:"segment,omitempty"`
}

type ClusterRestoreConfig struct {
	BackupWindowStart bmodels.OptionalBackupWindowStart `json:"-"`
	ZoneID            string                            `json:"zone_id,omitempty"`
	SubnetID          string                            `json:"subnet_id,omitempty"`
	AssignPublicIP    bool                              `json:"assign_public_ip,omitempty"`
	Access            *AccessSettings                   `json:"-"`
}

type ConfigSpec struct {
	Config ConfigBase
	Pool   PoolerConfigBase
}

type GPDBConfig struct {
	LogLevel                 int32  `json:"log_level"`
	MaxConnectionsDepricated *int64 `json:"max_connections"`

	MaxSlotWalKeepSize           *int64   `json:"max_slot_wal_keep_size,omitempty"`
	GPWorkfileLimitPerSegment    *int64   `json:"gp_workfile_limit_per_segment,omitempty"`
	GPWorkfileLimitPerQuery      *int64   `json:"gp_workfile_limit_per_query,omitempty"`
	GPWorkfileLimitFilesPerQuery *int64   `json:"gp_workfile_limit_files_per_query,omitempty"`
	GPResourceManager            *string  `json:"gp_resource_manager,omitempty"`
	GPResourceGroupCPULimit      *float32 `json:"gp_resource_group_cpu_limit,omitempty"`
	GPResourceGroupMemoryLimit   *float32 `json:"gp_resource_group_memory_limit,omitempty"`
}

type ClusterConfig struct {
	Version                     string                            `json:"version,omitempty"`
	BackupWindowStart           bmodels.OptionalBackupWindowStart `json:"-"`
	ZoneID                      string                            `json:"zone_id,omitempty"`
	SubnetID                    string                            `json:"subnet_id,omitempty"`
	AssignPublicIP              bool                              `json:"assign_public_ip,omitempty"`
	Access                      *AccessSettings                   `json:"-"`
	SegmentMirroringEnable      bool                              `json:"segment_mirroring_enable"`
	SegmentAutoRebalanceDisable bool                              `json:"segment_auto_rebalance_disable"`

	GPResourceManager                *string              `json:"gp_resource_manager,omitempty"`                  // 'queue' || 'group'
	SharedBuffers                    *string              `json:"shared_buffers,omitempty"`                       // '128MB'
	TempBuffers                      *string              `json:"temp_buffers,omitempty"`                         // '32MB'
	MaintenanceWorkMem               *string              `json:"maintenance_work_mem,omitempty"`                 // '64MB'
	WalLevel                         *string              `json:"wal_level,omitempty"`                            // 'archive'
	ArchiveMode                      *string              `json:"archive_mode,omitempty"`                         // 'on'
	ArchiveTimeout                   *int64               `json:"archive_timeout,omitempty"`                      // 600
	ArchiveCommand                   *string              `json:"archive_command,omitempty"`                      // null
	LogMinMessages                   *string              `json:"log_min_messages,omitempty"`                     // 'warning'
	LogMinErrorStatement             *string              `json:"log_min_error_statement,omitempty"`              // 'error'
	LogMinDurationStatement          *int64               `json:"log_min_duration_statement,omitempty"`           // -1
	LogCheckpoints                   *bool                `json:"log_checkpoints,omitempty"`                      // true
	LogConnections                   *bool                `json:"log_connections,omitempty"`                      // false
	LogDisconnections                *string              `json:"log_disconnections,omitempty"`                   // false
	LogErrorVerbosity                *string              `json:"log_error_verbosity,omitempty"`                  // 'default'
	LogHostname                      *bool                `json:"log_hostname,omitempty"`                         // false
	GPPerfmonEnable                  *bool                `json:"gpperfmon_enable,omitempty"`                     // false
	GPPerfmonPort                    *int64               `json:"gpperfmon_port,omitempty"`                       // 8888
	GPAutostatsMode                  *string              `json:"gp_autostats_mode,omitempty"`                    // 'on_no_stats'
	GPAutostatsOnChangeThreshold     *int64               `json:"gp_autostats_on_change_threshold,omitempty"`     // 2147483647
	LogAutostats                     *bool                `json:"log_autostats,omitempty"`                        // false
	TimeZone                         *string              `json:"timezone,omitempty"`                             // 'Europe/Moscow'
	MaxLocksPerTransaction           *int64               `json:"max_locks_per_transaction,omitempty"`            // 128
	MaxResourceQueues                *int64               `json:"max_resource_queues,omitempty"`                  // 9
	GPResqueueMemoryPolicy           *string              `json:"gp_resqueue_memory_policy,omitempty"`            // 'eager_free'
	GPExternalEnableExec             *bool                `json:"gp_external_enable_exec,omitempty"`              // false
	MaxAppendonlyTables              *int64               `json:"max_appendonly_tables,omitempty"`                // 10000
	GPMaxPacketSize                  *int64               `json:"gp_max_packet_size,omitempty"`                   // 8192
	GPInterconnectType               *string              `json:"gp_interconnect_type,omitempty"`                 // 'udpifc'
	GPSegmentConnectTimeout          *string              `json:"gp_segment_connect_timeout,omitempty"`           // '600s'
	GPVmemProtectLimit               *int64               `json:"gp_vmem_protect_limit,omitempty"`                // 8192
	GPVmemIdleResourceTimeout        *int64               `json:"gp_vmem_idle_resource_timeout,omitempty"`        // 18000
	MaxSlotWalKeepSize               *string              `json:"max_slot_wal_keep_size,omitempty"`               // '10GB'
	GPWorkfileLimitPerSegment        *string              `json:"gp_workfile_limit_per_segment,omitempty"`        // '10GB'
	GPWorkfileLimitPerQuery          *string              `json:"gp_workfile_limit_per_query,omitempty"`          // 0 | '150GB'
	GPWorkfileLimitFilesPerQuery     *int64               `json:"gp_workfile_limit_files_per_query,omitempty"`    // 10000
	GPInterconnectTCPListenerBacklog *int64               `json:"gp_interconnect_tcp_listener_backlog,omitempty"` // 65535
	MaxConnections                   *IntMasterSegment    `json:"max_connections,omitempty"`                      // 350 | null
	MaxPreparedTransactions          *int64               `json:"max_prepared_transactions,omitempty"`            // master_conn_limit
	RunawayDetectorActivationPercent *int64               `json:"runaway_detector_activation_percent,omitempty"`  // 90
	TCPKeepalivesCount               *int64               `json:"tcp_keepalives_count,omitempty"`                 // 6
	TCPKeepalivesInterval            *int64               `json:"tcp_keepalives_interval,omitempty"`              // 3
	GPResourceGroupCPULimit          *float32             `json:"gp_resource_group_cpu_limit,omitempty"`          // 0.9
	GPResourceGroupMemoryLimit       *float32             `json:"gp_resource_group_memory_limit,omitempty"`       // 0.7
	ReadableExternalTableTimeout     *int64               `json:"readable_external_table_timeout,omitempty"`      // 1800
	GPInterconnectSndQueueDepth      *int64               `json:"gp_interconnect_snd_queue_depth,omitempty"`      // 2
	GPInterconnectQueueDepth         *int64               `json:"gp_interconnect_queue_depth,omitempty"`          // 4
	Optimizer                        *bool                `json:"optimizer,omitempty"`                            // true
	LogStatement                     *StringMasterSegment `json:"log_statement,omitempty"`                        // 'ddl' | 'none'
	OptimizerAnalyzeRootPartition    *bool                `json:"optimizer_analyze_root_partition,omitempty"`     // true
	LogDuration                      *bool                `json:"log_duration,omitempty"`                         // false
	GPExternalMaxSegs                *int64               `json:"gp_external_max_segs,omitempty"`                 // 64
	GPFtsProbeTimeout                *string              `json:"gp_fts_probe_timeout,omitempty"`                 // '20s'
	GPWorkfileCompression            *bool                `json:"gp_workfile_compression,omitempty"`              // false
	GPAutostatsModeInFunctions       *string              `json:"gp_autostats_mode_in_functions,omitempty"`       // 'none'

	MaxStackDepth                        *string  `json:"max_stack_depth,omitempty"`                           // '2MB'
	MaxFilesPerProcess                   *int64   `json:"max_files_per_process,omitempty"`                     // 1000
	DebugAssertions                      *bool    `json:"debug_assertions,omitempty"`                          // false
	LogDispatchStats                     *bool    `json:"log_dispatch_stats,omitempty"`                        // false
	LogExecutorStats                     *bool    `json:"log_executor_stats,omitempty"`                        // false
	DeadlockTimeout                      *string  `json:"deadlock_timeout,omitempty"`                          // '1s'
	GPSafefswritesize                    *int64   `json:"gp_safefswritesize,omitempty"`                        // 0
	GPVmemProtectSegworkerCacheLimit     *int64   `json:"gp_vmem_protect_segworker_cache_limit,omitempty"`     // 500
	TCPKeepalivesIdle                    *int64   `json:"tcp_keepalives_idle,omitempty"`                       // 0
	GPMaxLocalDistributedCache           *int64   `json:"gp_max_local_distributed_cache,omitempty"`            // 1024
	GPResgroupMemoryPolicy               *string  `json:"gp_resgroup_memory_policy,omitempty"`                 // 'eager_free'
	GPResourceGroupCPUCeilingEnforcement *bool    `json:"gp_resource_group_cpu_ceiling_enforcement,omitempty"` // false
	GPResqueuePriority                   *bool    `json:"gp_resqueue_priority,omitempty"`                      // true
	GPResqueuePriorityCPUCoresPerSegment *float32 `json:"gp_resqueue_priority_cpucores_per_segment,omitempty"` // 4.0
	GPResqueuePrioritySweeperInterval    *int64   `json:"gp_resqueue_priority_sweeper_interval,omitempty"`     // 1000
	GPSnapshotaddTimeout                 *string  `json:"gp_snapshotadd_timeout,omitempty"`                    // '10s'
	GPFtsProbeInterval                   *string  `json:"gp_fts_probe_interval,omitempty"`                     // '60s'
	GPFtsProbeRetries                    *int64   `json:"gp_fts_probe_retries,omitempty"`                      // 5
	GPLogFts                             *string  `json:"gp_log_fts,omitempty"`                                // 'TERSE'
	GPFtsReplicationAttemptCount         *int64   `json:"gp_fts_replication_attempt_count,omitempty"`          // 10
	GPGlobalDeadlockDetectorPeriod       *string  `json:"gp_global_deadlock_detector_period,omitempty"`        // '120s'
	GPSetProcAffinity                    *bool    `json:"gp_set_proc_affinity,omitempty"`                      // false
	DtxPhase2RetryCount                  *int64   `json:"dtx_phase2_retry_count,omitempty"`                    // 10
	GPConnectionSendTimeout              *int64   `json:"gp_connection_send_timeout,omitempty"`                // 3600
	GPEnableDirectDispatch               *bool    `json:"gp_enable_direct_dispatch,omitempty"`                 // true
	GPEnableGlobalDeadlockDetector       *bool    `json:"gp_enable_global_deadlock_detector,omitempty"`        // false
	MaxResourcePortalsPerTransaction     *int64   `json:"max_resource_portals_per_transaction,omitempty"`      // 64
	ResourceCleanupGangsOnWait           *bool    `json:"resource_cleanup_gangs_on_wait,omitempty"`            // true
	MaxStatementMem                      *string  `json:"max_statement_mem,omitempty"`                         // 2000MB
}

type MasterSubclusterConfigSpec struct {
	Resources models.ClusterResources `json:"resources"`
	Config    ClusterMasterConfig     `json:"config"`
}
type SegmentSubclusterConfigSpec struct {
	Resources models.ClusterResources `json:"resources"`
	Config    ClusterSegmentConfig    `json:"config"`
}

type ClusterMasterConfig struct {
	LogLevel                         int32         `json:"log_level"`
	MaxConnectionsDepricated         *int64        `json:"max_connections"`
	TimeZone                         *string       `json:"timezone"`
	GPWorkfileLimitPerSegment        *string       `json:"gp_workfile_limit_per_segment,omitempty"`     // '10GB'
	GPWorkfileLimitPerQuery          *string       `json:"gp_workfile_limit_per_query,omitempty"`       // 0 | '150GB'
	GPWorkfileLimitFilesPerQuery     *int64        `json:"gp_workfile_limit_files_per_query,omitempty"` // 10000
	Pool                             *PoolerConfig `json:"pool,omitempty"`
	MaxSlotWalKeepSize               *string       `json:"max_slot_wal_keep_size,omitempty"`
	MaxPreparedTransactions          *int64        `json:"max_prepared_transactions,omitempty"`
	RunawayDetectorActivationPercent *int64        `json:"runaway_detector_activation_percent,omitempty"`
	TCPKeepalivesCount               *int64        `json:"tcp_keepalives_count,omitempty"`
	TCPKeepalivesInterval            *int64        `json:"tcp_keepalives_interval,omitempty"`
	ReadableExternalTableTimeout     *int64        `json:"readable_external_table_timeout,omitempty"`
	GPInterconnectSndQueueDepth      *int64        `json:"gp_interconnect_snd_queue_depth,omitempty"`
	GPInterconnectQueueDepth         *int64        `json:"gp_interconnect_queue_depth,omitempty"`
	LogStatement                     *string       `json:"log_statement,omitempty"`
	LogDuration                      *bool         `json:"log_duration,omitempty"`
	OptimizerAnalyzeRootPartition    *bool         `json:"optimizer_analyze_root_partition,omitempty"`
	GPExternalMaxSegs                *int64        `json:"gp_external_max_segs,omitempty"`
	GPFtsProbeTimeout                *int64        `json:"gp_fts_probe_timeout,omitempty"`
	GPWorkfileCompression            *bool         `json:"gp_workfile_compression,omitempty"`
	GPAutostatsModeInFunctions       *string       `json:"gp_autostats_mode_in_functions,omitempty"`
}
type ClusterSegmentConfig struct {
	LogLevel                 int32  `json:"log_level"`
	MaxConnectionsDepricated *int64 `json:"max_connections"`

	MaxSlotWalKeepSize           *int64   `json:"max_slot_wal_keep_size,omitempty"`
	GPWorkfileLimitPerSegment    *int64   `json:"gp_workfile_limit_per_segment,omitempty"`
	GPWorkfileLimitPerQuery      *int64   `json:"gp_workfile_limit_per_query,omitempty"`
	GPWorkfileLimitFilesPerQuery *int64   `json:"gp_workfile_limit_files_per_query,omitempty"`
	GPResourceManager            *string  `json:"gp_resource_manager,omitempty"`
	GPResourceGroupCPULimit      *float32 `json:"gp_resource_group_cpu_limit,omitempty"`
	GPResourceGroupMemoryLimit   *float32 `json:"gp_resource_group_memory_limit,omitempty"`
}

type AccessSettings struct {
	DataLens     bool `json:"data_lens"`
	WebSQL       bool `json:"web_sql"`
	DataTransfer bool `json:"data_transfer"`
	Serverless   bool `json:"serverless"`
}

type ClusterMasterConfigSet struct {
	EffectiveConfig ClusterMasterConfig
	DefaultConfig   ClusterMasterConfig
	UserConfig      ClusterMasterConfig
}
type ClusterSegmentConfigSet struct {
	EffectiveConfig ClusterSegmentConfig
	DefaultConfig   ClusterSegmentConfig
	UserConfig      ClusterSegmentConfig
}

type MasterConfig struct {
	Resources models.ClusterResources `json:"resources"`
	Config    ClusterMasterConfigSet  `json:"config"`
}
type SegmentConfig struct {
	Resources models.ClusterResources `json:"resources"`
	Config    ClusterSegmentConfigSet `json:"config"`
}

type ClusterConfigSet struct {
	EffectiveConfig ClusterConfig
	DefaultConfig   ClusterConfig
	UserConfig      ClusterConfig
}

type ClusterGPDBConfigSet struct {
	EffectiveConfig ConfigBase
	DefaultConfig   ConfigBase
	UserConfig      ConfigBase
}

type ClusterGpConfigSet struct {
	Pooler PoolerConfigSet
	Config ClusterGPDBConfigSet
}

type PoolerConfigSet struct {
	EffectiveConfig PoolerConfig
	DefaultConfig   PoolerConfig
	UserConfig      PoolerConfig
}

func ClusterConfigMerge(base ClusterConfig, add ClusterConfig) (ClusterConfig, error) {
	var mergeResult ClusterConfig

	mergeResult.MaxConnections = &IntMasterSegment{}
	if base.MaxConnections != nil {
		mergeResult.MaxConnections.Master = base.MaxConnections.Master
		mergeResult.MaxConnections.Segment = base.MaxConnections.Segment
	}
	if add.MaxConnections != nil {
		if add.MaxConnections.Master != nil {
			mergeResult.MaxConnections.Master = add.MaxConnections.Master
		}
		if add.MaxConnections.Segment != nil {
			mergeResult.MaxConnections.Segment = add.MaxConnections.Segment
		}
	}

	mergeResult.MaxPreparedTransactions = base.MaxPreparedTransactions
	if add.MaxPreparedTransactions != nil {
		mergeResult.MaxPreparedTransactions = add.MaxPreparedTransactions
	}
	mergeResult.RunawayDetectorActivationPercent = base.RunawayDetectorActivationPercent
	if add.RunawayDetectorActivationPercent != nil {
		mergeResult.RunawayDetectorActivationPercent = add.RunawayDetectorActivationPercent
	}
	mergeResult.TCPKeepalivesCount = base.TCPKeepalivesCount
	if add.TCPKeepalivesCount != nil {
		mergeResult.TCPKeepalivesCount = add.TCPKeepalivesCount
	}
	mergeResult.TCPKeepalivesInterval = base.TCPKeepalivesInterval
	if add.TCPKeepalivesInterval != nil {
		mergeResult.TCPKeepalivesInterval = add.TCPKeepalivesInterval
	}
	mergeResult.ReadableExternalTableTimeout = base.ReadableExternalTableTimeout
	if add.ReadableExternalTableTimeout != nil {
		mergeResult.ReadableExternalTableTimeout = add.ReadableExternalTableTimeout
	}
	mergeResult.GPInterconnectSndQueueDepth = base.GPInterconnectSndQueueDepth
	if add.GPInterconnectSndQueueDepth != nil {
		mergeResult.GPInterconnectSndQueueDepth = add.GPInterconnectSndQueueDepth
	}
	mergeResult.GPInterconnectQueueDepth = base.GPInterconnectQueueDepth
	if add.GPInterconnectQueueDepth != nil {
		mergeResult.GPInterconnectQueueDepth = add.GPInterconnectQueueDepth
	}
	mergeResult.LogStatement = base.LogStatement
	if add.LogStatement != nil {
		mergeResult.LogStatement = add.LogStatement
	}
	mergeResult.LogDuration = base.LogDuration
	if add.LogDuration != nil {
		mergeResult.LogDuration = add.LogDuration
	}
	mergeResult.OptimizerAnalyzeRootPartition = base.OptimizerAnalyzeRootPartition
	if add.OptimizerAnalyzeRootPartition != nil {
		mergeResult.OptimizerAnalyzeRootPartition = add.OptimizerAnalyzeRootPartition
	}
	mergeResult.GPExternalMaxSegs = base.GPExternalMaxSegs
	if add.GPExternalMaxSegs != nil {
		mergeResult.GPExternalMaxSegs = add.GPExternalMaxSegs
	}
	mergeResult.GPFtsProbeTimeout = base.GPFtsProbeTimeout
	if add.GPFtsProbeTimeout != nil {
		mergeResult.GPFtsProbeTimeout = add.GPFtsProbeTimeout
	}
	mergeResult.GPWorkfileCompression = base.GPWorkfileCompression
	if add.GPWorkfileCompression != nil {
		mergeResult.GPWorkfileCompression = add.GPWorkfileCompression
	}
	mergeResult.GPAutostatsModeInFunctions = base.GPAutostatsModeInFunctions
	if add.GPAutostatsModeInFunctions != nil {
		mergeResult.GPAutostatsModeInFunctions = add.GPAutostatsModeInFunctions
	}

	mergeResult.MaxSlotWalKeepSize = base.MaxSlotWalKeepSize
	if add.MaxSlotWalKeepSize != nil {
		mergeResult.MaxSlotWalKeepSize = add.MaxSlotWalKeepSize
	}
	mergeResult.GPWorkfileLimitPerSegment = base.GPWorkfileLimitPerSegment
	if add.GPWorkfileLimitPerSegment != nil {
		mergeResult.GPWorkfileLimitPerSegment = add.GPWorkfileLimitPerSegment
	}
	mergeResult.GPWorkfileLimitPerQuery = base.GPWorkfileLimitPerQuery
	if add.GPWorkfileLimitPerQuery != nil {
		mergeResult.GPWorkfileLimitPerQuery = add.GPWorkfileLimitPerQuery
	}
	mergeResult.GPWorkfileLimitFilesPerQuery = base.GPWorkfileLimitFilesPerQuery
	if add.GPWorkfileLimitFilesPerQuery != nil {
		mergeResult.GPWorkfileLimitFilesPerQuery = add.GPWorkfileLimitFilesPerQuery
	}
	mergeResult.GPResourceManager = base.GPResourceManager
	if add.GPResourceManager != nil {
		mergeResult.GPResourceManager = add.GPResourceManager
	}
	mergeResult.GPResourceGroupCPULimit = base.GPResourceGroupCPULimit
	if add.GPResourceGroupCPULimit != nil {
		mergeResult.GPResourceGroupCPULimit = add.GPResourceGroupCPULimit
	}
	mergeResult.GPResourceGroupMemoryLimit = base.GPResourceGroupMemoryLimit
	if add.GPResourceGroupMemoryLimit != nil {
		mergeResult.GPResourceGroupMemoryLimit = add.GPResourceGroupMemoryLimit
	}
	mergeResult.MaxStatementMem = base.MaxStatementMem
	if add.MaxStatementMem != nil {
		mergeResult.MaxStatementMem = add.MaxStatementMem
	}

	return mergeResult, nil
}

func PoolConfigMerge(base *PoolerConfig, add *PoolerConfig) (PoolerConfig, error) {
	var mergeResult PoolerConfig

	if base == nil && add != nil {
		mergeResult = *add
	}
	if base != nil && add == nil {
		mergeResult = *base
	}
	if base != nil && add != nil {
		mergeResult = PoolerConfig{
			Mode:              base.Mode,
			PoolSize:          base.PoolSize,
			ClientIdleTimeout: base.ClientIdleTimeout,
		}

		if add.Mode != nil {
			mergeResult.Mode = add.Mode
		}
		if add.PoolSize != nil {
			mergeResult.PoolSize = add.PoolSize
		}
		if add.ClientIdleTimeout != nil {
			mergeResult.ClientIdleTimeout = add.ClientIdleTimeout
		}
	}
	return mergeResult, nil
}

func ClusterConfigMasterMerge(base ClusterMasterConfig, add ClusterMasterConfig) (ClusterMasterConfig, error) {
	var mergeResult ClusterMasterConfig

	mergeResult.LogLevel = base.LogLevel
	if add.LogLevel != 0 {
		mergeResult.LogLevel = add.LogLevel
	}

	mergeResult.TimeZone = base.TimeZone
	if add.TimeZone != nil {
		mergeResult.TimeZone = add.TimeZone
	}

	mergeResult.MaxConnectionsDepricated = base.MaxConnectionsDepricated
	if add.MaxConnectionsDepricated != nil {
		mergeResult.MaxConnectionsDepricated = add.MaxConnectionsDepricated
	}

	return mergeResult, nil
}

func ClusterConfigSegmentMerge(base ClusterSegmentConfig, add ClusterSegmentConfig) (ClusterSegmentConfig, error) {
	var mergeResult ClusterSegmentConfig

	mergeResult.LogLevel = base.LogLevel
	if add.LogLevel != 0 {
		mergeResult.LogLevel = add.LogLevel
	}

	return mergeResult, nil
}
