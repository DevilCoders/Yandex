package rpillars

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
)

type RedisConfig struct {
	Databases                      int               `json:"databases,omitempty" name:"databases"`
	Save                           string            `json:"save,omitempty" name:"save"`
	Maxmemory                      int64             `json:"maxmemory,omitempty" name:"maxmemory"`
	Appendonly                     string            `json:"appendonly,omitempty" name:"appendonly"`
	Masterauth                     pillars.CryptoKey `json:"masterauth,omitempty" name:"masterauth"`
	Requirepass                    pillars.CryptoKey `json:"requirepass,omitempty" name:"requirepass"`
	IsShardedStr                   string            `json:"cluster-enabled,omitempty" name:"cluster-enabled"`
	ReplBacklogSize                int               `json:"repl-backlog-size,omitempty" name:"repl-backlog-size"`
	ClientOutputBufferLimitNormal  string            `json:"client-output-buffer-limit normal,omitempty" name:"client-output-buffer-limit-normal"`
	ClientOutputBufferLimitPubsub  string            `json:"client-output-buffer-limit pubsub,omitempty" name:"client-output-buffer-limit-pubsub"`
	ClientOutputBufferLimitReplica string            `json:"client-output-buffer-limit replica,omitempty" name:"client-output-buffer-limit-replica"`
}

type RedisSecrets struct {
	Renames               RedisRenames               `json:"renames"`
	ClusterRenames        RedisClusterRenames        `json:"cluster_renames,omitempty"`
	SentinelRenames       RedisSentinelRenames       `json:"sentinel_renames,omitempty"`
	SentinelDirectRenames RedisSentinelDirectRenames `json:"sentinel_direct_renames,omitempty"`
}

type RedisClusterRenames struct {
	Meet           pillars.CryptoKey `json:"MEET"`
	Reset          pillars.CryptoKey `json:"RESET"`
	Forget         pillars.CryptoKey `json:"FORGET"`
	SetSlot        pillars.CryptoKey `json:"SETSLOT"`
	AddSlots       pillars.CryptoKey `json:"ADDSLOTS"`
	DelSlots       pillars.CryptoKey `json:"DELSLOTS"`
	Failover       pillars.CryptoKey `json:"FAILOVER"`
	BumpEpoch      pillars.CryptoKey `json:"BUMPEPOCH"`
	Replicate      pillars.CryptoKey `json:"REPLICATE"`
	FlushSlots     pillars.CryptoKey `json:"FLUSHSLOTS"`
	SaveConfig     pillars.CryptoKey `json:"SAVECONFIG"`
	SetConfigEpoch pillars.CryptoKey `json:"SET-CONFIG-EPOCH"`
}

type RedisSentinelRenames struct {
	Failover  pillars.CryptoKey `json:"FAILOVER"`
	Reset     pillars.CryptoKey `json:"RESET"`
	ConfigSet pillars.CryptoKey `json:"CONFIG-SET,omitempty"`
}

type RedisSentinelDirectRenames struct {
	ACL pillars.CryptoKey `json:"ACL,omitempty"`
}

type RedisRenames struct {
	ACL          pillars.CryptoKey `json:"ACL,omitempty"`
	Move         pillars.CryptoKey `json:"MOVE"`
	Save         pillars.CryptoKey `json:"SAVE"`
	Debug        pillars.CryptoKey `json:"DEBUG"`
	BgSave       pillars.CryptoKey `json:"BGSAVE"`
	Config       pillars.CryptoKey `json:"CONFIG"`
	Module       pillars.CryptoKey `json:"MODULE"`
	Object       pillars.CryptoKey `json:"OBJECT"`
	Migrate      pillars.CryptoKey `json:"MIGRATE"`
	Monitor      pillars.CryptoKey `json:"MONITOR"`
	SlaveOf      pillars.CryptoKey `json:"SLAVEOF"`
	LastSave     pillars.CryptoKey `json:"LASTSAVE"`
	Shutdown     pillars.CryptoKey `json:"SHUTDOWN"`
	ReplicaOf    pillars.CryptoKey `json:"REPLICAOF"`
	BgRewriteAOF pillars.CryptoKey `json:"BGREWRITEAOF"`
}
