package clusters

import (
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/monitoring"
)

// Cluster holds basic cluster info
type Cluster struct {
	ClusterID          string
	Name               string
	Type               Type
	Environment        environment.SaltEnv
	CreatedAt          time.Time
	NetworkID          string
	SecurityGroupIDs   []string
	HostGroupIDs       []string
	Status             Status
	Description        string
	Labels             map[string]string
	Revision           int64
	BackupSchedule     backups.BackupSchedule
	DeletionProtection bool
}

// ClusterExtended holds extended cluster info
// TODO: think about naming
type ClusterExtended struct {
	Cluster
	FolderExtID     string
	Health          Health
	Monitoring      monitoring.Monitoring
	Pillar          json.RawMessage
	MaintenanceInfo MaintenanceInfo
}

type SubCluster struct {
	SubClusterID string
	ClusterID    string
	Name         string
	Roles        []hosts.Role
}

type KubernetesSubCluster struct {
	SubCluster          SubCluster
	KubernetesClusterID string
	NodeGroupID         string
}

type Shard struct {
	SubClusterID string
	ShardID      string
	Name         string
	CreatedAt    time.Time
}

type ShardExtended struct {
	Shard
	Pillar json.RawMessage
}

type ClusterPageToken struct {
	LastClusterName string
	More            bool `json:"-"`
}

func (cpt ClusterPageToken) HasMore() bool {
	return cpt.More
}

func NewClusterPageToken(actualSize int64, expectedPageSize int64, lastClusterName string) ClusterPageToken {
	var nextClusterPageToken ClusterPageToken
	if actualSize == expectedPageSize {
		nextClusterPageToken = ClusterPageToken{
			LastClusterName: lastClusterName,
			More:            true,
		}
	}

	return nextClusterPageToken
}
