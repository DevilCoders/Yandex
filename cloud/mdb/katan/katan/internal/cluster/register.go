package cluster

import (
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/clickhouse"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/greenplum"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/kafka"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/mongodb"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/mysql"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/postgresql"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/redis"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/sqlserver"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
)

var typedPlanners = make(map[string]planner.RolloutPlanner)

func registerUpdatePlanner(metaClusterType string, handle planner.RolloutPlanner) {
	if _, ok := typedPlanners[metaClusterType]; ok {
		panic("update planner for " + metaClusterType + " already registered")
	}
	typedPlanners[metaClusterType] = handle
}

func init() {
	registerUpdatePlanner("postgresql_cluster", postgresql.Planner)
	registerUpdatePlanner("mongodb_cluster", mongodb.Planner)
	registerUpdatePlanner("mysql_cluster", mysql.Planner)
	registerUpdatePlanner("redis_cluster", redis.Planner)
	registerUpdatePlanner("sqlserver_cluster", sqlserver.Planner)
	registerUpdatePlanner("kafka_cluster", kafka.Planner)
	registerUpdatePlanner("clickhouse_cluster", clickhouse.Planner)
	registerUpdatePlanner("greenplum_cluster", greenplum.Planner)
}

// GetPlanner returns rollout planner for the cluster
func GetPlanner(cluster models.Cluster) planner.RolloutPlanner {
	if len(cluster.Hosts) == 1 {
		// We don't need a custom plan for clusters that have only one host.
		// Such clusters have only one rollout plan.
		return func(cluster planner.Cluster) ([][]string, error) {
			for h := range cluster.Hosts {
				return [][]string{{h}}, nil
			}
			return nil, nil
		}
	}
	if len(cluster.Tags.Meta.Type) > 0 {
		if pk, ok := typedPlanners[string(cluster.Tags.Meta.Type)]; ok {
			return pk
		}
	}
	return planner.Default
}
