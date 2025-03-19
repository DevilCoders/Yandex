package chmodels

const (
	PasswordLen = 128

	CHSubClusterName = "clickhouse_subcluster"
	ZKSubClusterName = "zookeeper_subcluster"

	ZKACLUserSuper      = "super"
	ZKACLUserClickhouse = "clickhouse"
	ZKACLUserBackup     = "backup"

	SystemUserBackupAdmin = "mdb_backup_admin"

	ShardNameTemplate  = "s%d"
	DefaultShardName   = "shard1"
	DefaultShardWeight = 100
	ZooKeeperShardName = "zk"

	ClickHouseTestingVersionsFeatureFlag        = "MDB_CLICKHOUSE_TESTING_VERSIONS"
	ClickHouseUnlimitedShardCount               = "MDB_CLICKHOUSE_UNLIMITED_SHARD_COUNT"
	ClickHouseDisableClusterConfigurationChecks = "MDB_CLICKHOUSE_DISABLE_CLUSTER_CONFIGURATION_CHECKS"

	StandardFlavorType = "standard"

	ClickHouseSearchService = "managed-clickhouse"

	DefaultBackupTime        = "Mon Jan 1 1:00:00 2021"
	DefaultBackupIDSeparator = ","
)

var AdminPasswordPath = []string{"data", "clickhouse", "admin_password", "password"}
