package clusters

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Type int

const (
	TypeUnknown Type = iota
	TypeClickHouse
	TypeElasticSearch
	TypeOpenSearch
	TypeHadoop
	TypeKafka
	TypeMongoDB
	TypeMySQL
	TypePostgreSQL
	TypeRedis
	TypeSQLServer
	TypeGreenplumCluster
	TypeAirflow
	TypeMetastore
)

type Info struct {
	Name             string
	Stringified      string
	ShortStringified string
	ResourceName     string
	Roles            Roles
}

var typeMapping = map[Type]Info{
	TypeClickHouse: {
		Name:             "ClickHouse",
		Stringified:      "clickhouse_cluster",
		ShortStringified: "ch",
		ResourceName:     "managed-clickhouse.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleClickHouse,
				hosts.RoleZooKeeper,
			},
			Main: hosts.RoleClickHouse,
		},
	},
	TypeElasticSearch: {
		Name:         "ElasticSearch",
		Stringified:  "elasticsearch_cluster",
		ResourceName: "managed-elasticsearch.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleElasticSearchDataNode,
				hosts.RoleElasticSearchMasterNode,
			},
			Main: hosts.RoleElasticSearchDataNode,
		},
	},
	TypeOpenSearch: {
		Name:         "OpenSearch",
		Stringified:  "opensearch_cluster",
		ResourceName: "managed-opensearch.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleOpenSearchDataNode,
				hosts.RoleOpenSearchMasterNode,
			},
			Main: hosts.RoleOpenSearchDataNode,
		},
	},
	TypeHadoop: {
		Name:         "Hadoop",
		Stringified:  "hadoop_cluster",
		ResourceName: "dataproc.cluster",
	},
	TypeKafka: {
		Name:             "Kafka",
		Stringified:      "kafka_cluster",
		ShortStringified: "kf",
		ResourceName:     "managed-kafka.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleKafka,
				hosts.RoleZooKeeper,
			},
			Main: hosts.RoleKafka,
		},
	},
	TypeMongoDB: {
		Name:         "MongoDB",
		Stringified:  "mongodb_cluster",
		ResourceName: "managed-mongodb.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleMongoD,
				hosts.RoleMongoS,
				hosts.RoleMongoCfg,
				hosts.RoleMongoInfra,
			},
			Main: hosts.RoleMongoD,
		},
	},
	TypeMySQL: {
		Name:         "MySQL",
		Stringified:  "mysql_cluster",
		ResourceName: "managed-mysql.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleMySQL,
			},
			Main: hosts.RoleMySQL,
		},
	},
	TypePostgreSQL: {
		Name:         "PostgreSQL",
		Stringified:  "postgresql_cluster",
		ResourceName: "managed-postgresql.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RolePostgreSQL,
			},
			Main: hosts.RolePostgreSQL,
		},
	},
	TypeRedis: {
		Name:         "Redis",
		Stringified:  "redis_cluster",
		ResourceName: "managed-redis.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleRedis,
			},
			Main: hosts.RoleRedis,
		},
	},
	TypeSQLServer: {
		Name:         "SQLServer",
		Stringified:  "sqlserver_cluster",
		ResourceName: "managed-sqlserver.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleSQLServer,
			},
			Main: hosts.RoleSQLServer,
		},
	},
	TypeGreenplumCluster: {
		Name:         "Greenplum",
		Stringified:  "greenplum_cluster",
		ResourceName: "managed-greenplum.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleGreenplumMasterNode,
				hosts.RoleGreenplumSegmentNode,
			},
			Main: hosts.RoleGreenplumMasterNode,
		},
	},
	TypeAirflow: {
		Name:         "Airflow",
		Stringified:  "airflow_cluster",
		ResourceName: "managed-airflow.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleAirflow,
			},
			Main: hosts.RoleAirflow,
		},
	},
	TypeMetastore: {
		Name:             "Metastore",
		Stringified:      "metastore_cluster",
		ShortStringified: "ms",
		ResourceName:     "managed-metastore.cluster",
		Roles: Roles{
			Possible: []hosts.Role{
				hosts.RoleMetastore,
			},
			Main: hosts.RoleMetastore,
		},
	},
}

var (
	stringifiedToTypeMapping = make(map[string]Type, len(typeMapping))
	nameToTypeMapping        = make(map[string]Type, len(typeMapping))
	types                    = make([]Type, len(typeMapping))
)

func init() {
	for typ, info := range typeMapping {
		if _, ok := stringifiedToTypeMapping[info.Stringified]; ok {
			panic(fmt.Sprintf("duplicate stringified cluster type %q", info.Stringified))
		}
		stringifiedToTypeMapping[info.Stringified] = typ

		if _, ok := nameToTypeMapping[info.Name]; ok {
			panic(fmt.Sprintf("duplicate cluster type name %q", info.Name))
		}
		nameToTypeMapping[strings.ToLower(info.Name)] = typ

		types = append(types, typ)
	}
}

func Types() []Type {
	return types[:]
}

func (t *Type) UnmarshalJSON(data []byte) error {
	typ, err := ParseTypeName(string(data))
	if err != nil {
		return err
	}

	*t = typ
	return nil
}

func (t *Type) UnmarshalYAML(unmarshal func(interface{}) error) error {
	var s string
	if err := unmarshal(&s); err != nil {
		return err
	}

	typ, err := ParseTypeName(s)
	if err != nil {
		return err
	}

	*t = typ
	return nil
}

func ParseTypeName(s string) (Type, error) {
	typ, ok := nameToTypeMapping[s]
	if !ok {
		return TypeUnknown, xerrors.Errorf("unknown cluster type name %q", s)
	}
	return typ, nil
}

func ParseTypeStringified(s string) (Type, error) {
	typ, ok := stringifiedToTypeMapping[s]
	if !ok {
		return TypeUnknown, xerrors.Errorf("unknown stringified cluster type %q", s)
	}
	return typ, nil
}

func (t Type) String() string {
	info, ok := typeMapping[t]
	if !ok {
		return fmt.Sprintf("UNKNOWN_CLUSTER_TYPE_%d", t)
	}

	return info.Name
}

func (t Type) Stringified() string {
	return t.Info().Stringified
}

// ShortStringified used as part of the DataCloud host FQDN
func (t Type) ShortStringified() string {
	return t.Info().ShortStringified
}

func (t Type) Roles() Roles {
	return t.Info().Roles
}

func (t Type) Info() Info {
	return typeMapping[t]
}
