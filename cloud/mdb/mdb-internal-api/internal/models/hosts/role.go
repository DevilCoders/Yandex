package hosts

import (
	"fmt"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type Role int

const (
	RoleUnknown Role = iota
	RoleClickHouse
	RoleElasticSearchDataNode
	RoleElasticSearchMasterNode
	RoleOpenSearchDataNode
	RoleOpenSearchMasterNode
	RoleKafka
	RoleMongoD
	RoleMongoS
	RoleMongoCfg
	RoleMongoInfra
	RoleMySQL
	RolePostgreSQL
	RoleRedis
	RoleSQLServer
	RoleZooKeeper
	RoleGreenplumSegmentNode
	RoleGreenplumMasterNode
	RoleWindowsWitnessNode
	RoleMetastore
	RoleAirflow
)

type Info struct {
	Name        string
	Stringified string
}

var roleMapping = map[Role]Info{
	RoleClickHouse: {
		Name:        "ClickHouse",
		Stringified: "clickhouse_cluster",
	},
	RoleElasticSearchDataNode: {
		Name:        "ElasticSearch data node",
		Stringified: "elasticsearch_cluster.datanode",
	},
	RoleElasticSearchMasterNode: {
		Name:        "ElasticSearch master node",
		Stringified: "elasticsearch_cluster.masternode",
	},
	RoleOpenSearchDataNode: {
		Name:        "OpenSearch data node",
		Stringified: "opensearch_cluster.datanode",
	},
	RoleOpenSearchMasterNode: {
		Name:        "OpenSearch master node",
		Stringified: "opensearch_cluster.masternode",
	},
	RoleKafka: {
		Name:        "Kafka",
		Stringified: "kafka_cluster",
	},
	RoleMongoD: {
		Name:        "Mongod",
		Stringified: "mongodb_cluster.mongod",
	},
	RoleMongoS: {
		Name:        "Mongos",
		Stringified: "mongodb_cluster.mongos",
	},
	RoleMongoCfg: {
		Name:        "Mongocfg",
		Stringified: "mongodb_cluster.mongocfg",
	},
	RoleMongoInfra: {
		Name:        "Mongoinfra",
		Stringified: "mongodb_cluster.mongoinfra",
	},
	RoleMySQL: {
		Name:        "MySQL",
		Stringified: "mysql_cluster",
	},
	RolePostgreSQL: {
		Name:        "PostgreSQL",
		Stringified: "postgresql_cluster",
	},
	RoleRedis: {
		Name:        "Redis",
		Stringified: "redis_cluster",
	},
	RoleSQLServer: {
		Name:        "SQL Server",
		Stringified: "sqlserver_cluster",
	},
	RoleZooKeeper: {
		Name:        "ZooKeeper",
		Stringified: "zk",
	},
	RoleGreenplumMasterNode: {
		Name:        "Greenplum Master",
		Stringified: "greenplum_cluster.master_subcluster",
	},
	RoleGreenplumSegmentNode: {
		Name:        "Greenplum Segment",
		Stringified: "greenplum_cluster.segment_subcluster",
	},
	RoleWindowsWitnessNode: {
		Name:        "WindowsWitness",
		Stringified: "windows_witness",
	},
	RoleAirflow: {
		Name:        "Airflow Node group",
		Stringified: "airflow_cluster",
	},
	RoleMetastore: {
		Name:        "Metastore",
		Stringified: "metastore_cluster",
	},
}

var stringifiedToTypeMapping = make(map[string]Role, len(roleMapping))

func init() {
	for role, info := range roleMapping {
		if _, ok := stringifiedToTypeMapping[info.Stringified]; ok {
			panic(fmt.Sprintf("duplicate stringified host role %q", info.Stringified))
		}

		stringifiedToTypeMapping[info.Stringified] = role
	}
}

func ParseRole(s string) (Role, error) {
	typ, ok := stringifiedToTypeMapping[s]
	if !ok {
		return RoleUnknown, xerrors.Errorf("unknown stringified host role %q", s)
	}
	return typ, nil
}

func (r Role) String() string {
	info, ok := roleMapping[r]
	if !ok {
		return fmt.Sprintf("UNKNOWN_HOST_ROLE_%d", r)
	}

	return info.Name
}

func (r Role) Stringified() string {
	return r.Info().Stringified
}

func (r Role) Info() Info {
	return roleMapping[r]
}

func DedupRoles(roles []Role) []Role {
	uniqueRoles := make(map[string]Role)
	for _, role := range roles {
		uniqueRoles[role.String()] = role
	}

	var dedupRoles []Role
	for _, role := range uniqueRoles {
		dedupRoles = append(dedupRoles, role)
	}

	return dedupRoles
}
