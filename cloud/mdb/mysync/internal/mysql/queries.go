package mysql

const (
	queryPing                        = "ping"
	querySlaveStatus                 = "slave_status"
	queryGTIDExecuted                = "gtid_executed"
	queryShowBinaryLogs              = "binary_logs"
	querySlaveHosts                  = "slave_hosts"
	queryReplicationLag              = "replication_lag"
	queryIsReadOnly                  = "is_readonly"
	querySetReadonly                 = "set_readonly"
	querySetReadonlyNoSuper          = "set_readonly_no_super"
	querySetWritable                 = "set_writable"
	queryStopSlave                   = "stop_slave"
	queryStartSlave                  = "start_slave"
	queryStopSlaveIOThread           = "stop_slave_io_thread"
	queryStartSlaveIOThread          = "start_slave_io_thread"
	queryStopSlaveSQLThread          = "stop_slave_sql_thread"
	queryStartSlaveSQLThread         = "start_slave_sql_thread"
	queryResetSlaveAll               = "reset_slave_all"
	queryChangeMaster                = "change_master"
	querySemiSyncStatus              = "semisync_status"
	querySemiSyncSetMaster           = "semisync_set_master"
	querySemiSyncSetSlave            = "semisync_set_slave"
	querySemiSyncDisable             = "semisync_disable"
	querySetSemiSyncWaitSlaveCount   = "set_semisync_wait_slave_count"
	queryListSlavesideDisabledEvents = "list_slaveside_disabled_events"
	queryEnableEvent                 = "enable_event"
	querySetLockTimeout              = "set_lock_timeout"
	queryKillQuery                   = "kill_query"
	queryGetProcessIds               = "get_process_ids"
	queryEnableOfflineMode           = "enable_offline_mode"
	queryDisableOfflineMode          = "disable_offline_mode"
	queryGetOfflineMode              = "get_offline_mode"
	queryHasWaitingSemiSyncAck       = "has_waiting_semi_sync_ack"
)

var DefaultQueries = map[string]string{
	queryPing:                `SELECT 1 AS Ok`,
	querySlaveStatus:         `SHOW SLAVE STATUS`,
	queryGTIDExecuted:        `SELECT @@GLOBAL.gtid_executed  as Executed_Gtid_Set`,
	queryShowBinaryLogs:      `SHOW BINARY LOGS`,
	querySlaveHosts:          `SHOW SLAVE HOSTS`,
	queryReplicationLag:      `SHOW SLAVE STATUS`,
	queryIsReadOnly:          `SELECT @@read_only AS ReadOnly, @@super_read_only AS SuperReadOnly`,
	querySetReadonly:         `SET GLOBAL super_read_only = 1`, // @@read_only will be set automatically
	querySetReadonlyNoSuper:  `SET GLOBAL read_only = 1, super_read_only = 0`,
	querySetWritable:         `SET GLOBAL read_only = 0`, // @@super_read_only will be unset automatically
	queryStopSlave:           `STOP SLAVE`,
	queryStartSlave:          `START SLAVE`,
	queryStopSlaveIOThread:   `STOP SLAVE IO_THREAD`,
	queryStartSlaveIOThread:  `START SLAVE IO_THREAD`,
	queryStopSlaveSQLThread:  `STOP SLAVE SQL_THREAD`,
	queryStartSlaveSQLThread: `START SLAVE SQL_THREAD`,
	queryResetSlaveAll:       `RESET SLAVE ALL`,
	queryChangeMaster: `CHANGE MASTER TO
								MASTER_HOST = :host ,
								MASTER_PORT = :port ,
								MASTER_USER = :user ,
								MASTER_PASSWORD = :password ,
								MASTER_SSL = :ssl ,
								MASTER_SSL_CA = :sslCa ,
								MASTER_SSL_VERIFY_SERVER_CERT = 1,
								MASTER_AUTO_POSITION = 1,
								MASTER_CONNECT_RETRY = :connectRetry,
								MASTER_RETRY_COUNT = :retryCount,
								MASTER_HEARTBEAT_PERIOD = :heartbeatPeriod `,
	querySemiSyncStatus: `SELECT @@rpl_semi_sync_master_enabled AS MasterEnabled,
								 @@rpl_semi_sync_slave_enabled AS SlaveEnabled,
								 @@rpl_semi_sync_master_wait_for_slave_count as WaitSlaveCount`,
	querySemiSyncSetMaster:         `SET GLOBAL rpl_semi_sync_master_enabled = 1, rpl_semi_sync_slave_enabled = 0`,
	querySemiSyncSetSlave:          `SET GLOBAL rpl_semi_sync_slave_enabled = 1, rpl_semi_sync_master_enabled = 0`,
	querySemiSyncDisable:           `SET GLOBAL rpl_semi_sync_slave_enabled = 0, rpl_semi_sync_master_enabled = 0`,
	querySetSemiSyncWaitSlaveCount: `SET GLOBAL rpl_semi_sync_master_wait_for_slave_count = :wait_slave_count`,
	queryListSlavesideDisabledEvents: `SELECT EVENT_SCHEMA, EVENT_NAME
										FROM information_schema.EVENTS
										WHERE STATUS = 'SLAVESIDE_DISABLED'`,

	queryEnableEvent:           `ALTER EVENT :schema.:name ENABLE`,
	querySetLockTimeout:        `SET SESSION lock_wait_timeout = ?`,
	queryKillQuery:             `KILL :kill_id`,
	queryGetProcessIds:         `SELECT ID FROM information_schema.PROCESSLIST p WHERE USER NOT IN (?) AND COMMAND != 'Killed'`,
	queryEnableOfflineMode:     `SET GLOBAL offline_mode = ON`,
	queryDisableOfflineMode:    `SET GLOBAL offline_mode = OFF`,
	queryGetOfflineMode:        `SELECT @@GLOBAL.offline_mode AS OfflineMode`,
	queryHasWaitingSemiSyncAck: `SELECT count(*) <> 0 AS IsWaiting FROM information_schema.PROCESSLIST WHERE state = 'Waiting for semi-sync ACK from slave'`,
}
