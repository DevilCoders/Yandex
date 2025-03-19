package provider

import (
	"context"
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	hlth "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/health"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/support"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/support/clmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	hmodel "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/log"
)

type Support struct {
	mdb    metadb.Backend
	health hlth.Health
	ses    sessions.Sessions
	l      log.Logger
}

var _ support.Support = &Support{}

var componentMap = map[clusters.Type]string{
	clusters.TypePostgreSQL:       "postgres",
	clusters.TypeGreenplumCluster: "greenplum",
	clusters.TypeRedis:            "redis",
	clusters.TypeMySQL:            "mysql",
	clusters.TypeMongoDB:          "mongodb",
}

// pillar key for version info
var pillarVersionMap = map[clusters.Type]string{
	clusters.TypePostgreSQL: "pg",
	clusters.TypeMongoDB:    "mongodb",
	clusters.TypeRedis:      "redis",
	clusters.TypeMySQL:      "mysql",
	clusters.TypeSQLServer:  "sqlserver",
}

func getFromPillar(raw json.RawMessage, path []string) (string, bool) {
	var jmap map[string]json.RawMessage
	err := json.Unmarshal(raw, &jmap)
	if err != nil {
		return "", false
	}
	value, exist := jmap[path[0]]
	if !exist {
		return "", false
	}
	result := ""
	ok := false
	if len(path) > 1 {
		result, ok = getFromPillar(value, path[1:])
	} else {
		err = json.Unmarshal(value, &result)
		if err == nil {
			ok = true
		}
	}
	return result, ok
}

func getVersionFromPillar(pillar json.RawMessage, ctype clusters.Type) (string, bool) {
	pillarVersionKey, exist := pillarVersionMap[ctype]
	if !exist {
		return "", false
	}
	path := []string{"data", pillarVersionKey, "version", "major_human"}
	version, ok := getFromPillar(pillar, path)
	return version, ok
}

func (s *Support) ResolveCluster(ctx context.Context, cid string) (clmodels.ClusterResult, error) {
	ctx, _, err := s.ses.Begin(ctx, sessions.ResolveByDeletedCluster(cid, models.PermMDBAllSupport), sessions.WithPreferStandby())
	if err != nil {
		return clmodels.ClusterResult{}, err
	}
	defer s.ses.Rollback(ctx)

	cluster, err := s.mdb.ClusterByClusterID(ctx, cid, models.VisibilityAll)
	if err != nil {
		return clmodels.ClusterResult{}, err
	}

	folderCoords, _, _, err := s.mdb.FolderCoordsByClusterID(ctx, cid, models.VisibilityAll)
	if err != nil {
		return clmodels.ClusterResult{}, err
	}

	version := "unknown"
	pillarVersion, ok := getVersionFromPillar(cluster.Pillar, cluster.Type)
	if ok {
		version = pillarVersion
	}
	component, ok := componentMap[cluster.Type]
	if ok {
		versions, err := s.mdb.GetClusterVersions(ctx, cid)
		if err != nil {
			return clmodels.ClusterResult{}, err
		}
		for _, v := range versions {
			if v.Component == component {
				version = v.MajorVersion
			}
		}
	}

	hosts, _, _, err := s.mdb.ListHosts(ctx, cid, 0, optional.Int64{})
	if err != nil {
		return clmodels.ClusterResult{}, err
	}
	phosts := make([]*hmodel.Host, len(hosts))
	for i, h := range hosts {
		hcopy := h
		phosts[i] = &hcopy
	}

	chealth, err := s.health.Cluster(ctx, cid)
	if err != nil {
		return clmodels.ClusterResult{}, err
	}

	fqdns := make([]string, len(hosts))
	for i, h := range hosts {
		fqdns[i] = h.FQDN
	}
	hhealth, err := s.health.Hosts(ctx, fqdns)
	if err != nil {
		return clmodels.ClusterResult{}, err
	}

	healthInfo := clmodels.HealthInfo{
		Cluster: chealth,
		Hosts:   hhealth,
	}

	clext := clmodels.ClusterResult{
		Cluster:     cluster.Cluster,
		Health:      healthInfo,
		FolderCoord: folderCoords,
		Version:     version,
		Hosts:       phosts,
	}

	return clext, nil
}

func NewSupport(metaDB metadb.Backend, health hlth.Health, ses sessions.Sessions, l log.Logger) *Support {
	return &Support{
		mdb:    metaDB,
		health: health,
		ses:    ses,
		l:      l,
	}
}
