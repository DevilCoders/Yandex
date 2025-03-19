package logs

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

// Message from log
type Message struct {
	// Timestamp of message
	Timestamp time.Time
	// Message itself
	Message map[string]string
	// NextMessageToken for next message of the same logs query. Valid only in context of initial request.
	NextMessageToken int64
}

// ServiceType for log queries
type ServiceType int

const (
	ServiceTypeInvalid ServiceType = iota
	ServiceTypePostgreSQL
	ServiceTypePooler
	ServiceTypeClickHouse
	ServiceTypeMongoD
	ServiceTypeMongoS
	ServiceTypeMongoCFG
	ServiceTypeMongoDBAudit
	ServiceTypeMySQLGeneral
	ServiceTypeMySQLError
	ServiceTypeMySQLSlowQuery
	ServiceTypeMySQLAudit
	ServiceTypeRedis
	ServiceTypeElasticSearch
	ServiceTypeKibana
	ServiceTypeKafka
	ServiceTypeGreenplum
	ServiceTypeGreenplumPooler
	ServiceTypeOpenSearch
	ServiceTypeOpenSearchDashboards
)

type ServiceTypeInfo struct {
	Name        string
	ClusterType clusters.Type
}

var serviceTypes = map[ServiceType]ServiceTypeInfo{
	ServiceTypeInvalid:              {Name: "Invalid", ClusterType: clusters.TypeUnknown},
	ServiceTypePostgreSQL:           {Name: "PostgreSQL", ClusterType: clusters.TypePostgreSQL},
	ServiceTypePooler:               {Name: "Pooler", ClusterType: clusters.TypePostgreSQL},
	ServiceTypeClickHouse:           {Name: "ClickHouse", ClusterType: clusters.TypeClickHouse},
	ServiceTypeMongoD:               {Name: "MongoD", ClusterType: clusters.TypeMongoDB},
	ServiceTypeMongoS:               {Name: "MongoS", ClusterType: clusters.TypeMongoDB},
	ServiceTypeMongoCFG:             {Name: "MongoCFG", ClusterType: clusters.TypeMongoDB},
	ServiceTypeMongoDBAudit:         {Name: "MongoDBAudit", ClusterType: clusters.TypeMongoDB},
	ServiceTypeMySQLGeneral:         {Name: "MySQLGeneral", ClusterType: clusters.TypeMySQL},
	ServiceTypeMySQLError:           {Name: "MySQLError", ClusterType: clusters.TypeMySQL},
	ServiceTypeMySQLSlowQuery:       {Name: "MySQLSlowQuery", ClusterType: clusters.TypeMySQL},
	ServiceTypeMySQLAudit:           {Name: "MySQLAudit", ClusterType: clusters.TypeMySQL},
	ServiceTypeRedis:                {Name: "Redis", ClusterType: clusters.TypeRedis},
	ServiceTypeElasticSearch:        {Name: "ElasticSearch", ClusterType: clusters.TypeElasticSearch},
	ServiceTypeKibana:               {Name: "Kibana", ClusterType: clusters.TypeElasticSearch},
	ServiceTypeKafka:                {Name: "Kafka", ClusterType: clusters.TypeKafka},
	ServiceTypeGreenplum:            {Name: "Greenplum", ClusterType: clusters.TypeGreenplumCluster},
	ServiceTypeGreenplumPooler:      {Name: "GreenplumOdyssey", ClusterType: clusters.TypeGreenplumCluster},
	ServiceTypeOpenSearch:           {Name: "OpenSearch", ClusterType: clusters.TypeOpenSearch},
	ServiceTypeOpenSearchDashboards: {Name: "OpenSearchDashboards", ClusterType: clusters.TypeOpenSearch},
}

func (st ServiceType) String() string {
	info, ok := serviceTypes[st]
	if !ok {
		panic(fmt.Sprintf("impossible happened: invalid logs service type %d", st))
	}

	return info.Name
}

func (st ServiceType) ClusterType() clusters.Type {
	return st.Info().ClusterType
}

func (st ServiceType) Info() ServiceTypeInfo {
	return serviceTypes[st]
}
