package rpillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/rmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Cluster struct {
	Data ClusterData `json:"data"`
}

var _ pillars.Marshaler = &Cluster{}
var _ clusters.Pillar = &Cluster{}

func (c *Cluster) Validate() error {
	return nil
}

func (c *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal redis cluster pillar: %w", err)
	}

	return raw, err
}

func (c *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	if len(raw) == 0 {
		return nil
	}
	if err := json.Unmarshal(raw, c); err != nil {
		return xerrors.Errorf("failed to unmarshal redis cluster pillar: %w", err)
	}

	return nil
}

type ClusterData struct {
	pillars.ClusterData

	Access            *pillars.AccessSettings `json:"access,omitempty"`
	Backup            backups.BackupSchedule  `json:"backup"`
	ClusterPrivateKey pillars.CryptoKey       `json:"cluster_private_key"`
	S3                *S3                     `json:"s3,omitempty"`
	S3Bucket          string                  `json:"s3_bucket"`
	Redis             ClusterDataRedis        `json:"redis"`
}

type S3 struct {
	GPGKey   pillars.CryptoKey `json:"gpg_key"`
	GPGKeyID string            `json:"gpg_key_id"`
}

type ClusterDataRedis struct {
	Config         RedisConfig    `json:"config"`
	Secrets        RedisSecrets   `json:"secrets"`
	TLS            TLSData        `json:"tls"`
	ZkHosts        []string       `json:"zk_hosts"`
	MaxShardsCount optional.Int64 `json:"max_shards_count"`
}

type TLSData struct {
	Enabled bool `json:"enabled"`
}

func (c *Cluster) IsSharded() bool {
	return c.Data.Redis.Config.IsShardedStr == rmodels.ShardedTurnedOnStr
}

func (c *Cluster) MaxShardsNum() int64 {
	if c.Data.Redis.MaxShardsCount.Valid {
		return c.Data.Redis.MaxShardsCount.Must()
	}
	return rmodels.MaxShardsNum
}
