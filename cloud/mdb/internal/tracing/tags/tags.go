package tags

var (
	RequestID = StringTagName("request_id")

	Backend         = StringTagName("backend")
	BackupID        = StringTagName("backup.id")
	CloudExtID      = StringTagName("cloud.ext_id")
	ClusterEnv      = StringTagName("cluster.env")
	ClusterID       = StringTagName("cluster.id")
	ClusterType     = StringTagName("cluster.type")
	FolderExtID     = StringTagName("folder.ext_id")
	NetworkID       = StringTagName("network.id")
	OperationID     = StringTagName("operation.id")
	PublicKeyCached = BoolTagName("key.public.cached")
	ShardID         = StringTagName("shard.id")
	UserID          = StringTagName("user.id")
	UserType        = StringTagName("user.type")
)
