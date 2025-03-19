package clmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

type HealthInfo struct {
	Cluster clusters.Health
	Hosts   map[string]hosts.Health
}

type ClusterResult struct {
	Cluster     clusters.Cluster
	Health      HealthInfo
	FolderCoord metadb.FolderCoords
	Version     string
	Hosts       []*hosts.Host
}
