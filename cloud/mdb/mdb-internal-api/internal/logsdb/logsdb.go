package logsdb

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

//go:generate ../../../scripts/mockgen.sh Backend

type Backend interface {
	ready.Checker

	// Logs queries underlying storage for logs with specified filters and returns
	// them in FIFO order (older messages first and newer messages last). NextMessageToken
	// 'points' to the next message in line and can be used as offset for next paged query.
	Logs(
		ctx context.Context,
		cid string,
		st LogType,
		columnFilter []string,
		fromTS, toTS time.Time,
		limit, offset int64,
		conditions []sqlfilter.Term,
	) (logs []logs.Message, more bool, err error)
}

type LogType string

const (
	LogTypeUnknown          LogType = "UNKNOWN"
	LogTypePostgreSQL       LogType = "POSTGRESQL"
	LogTypePGBouncer        LogType = "PGBOUNCER"
	LogTypeOdyssey          LogType = "ODYSSEY"
	LogTypeClickHouse       LogType = "CLICKHOUSE"
	LogTypeMongoD           LogType = "MONGOD"
	LogTypeMongoS           LogType = "MONGOS"
	LogTypeMongoCFG         LogType = "MONGOCFG"
	LogTypeMongoDBAudit     LogType = "MONGODB_AUDIT"
	LogTypeMySQLGeneral     LogType = "MYSQL_GENERAL"
	LogTypeMySQLError       LogType = "MYSQL_ERROR"
	LogTypeMySQLSlowQuery   LogType = "MYSQL_SLOW_QUERY"
	LogTypeMySQLAudit       LogType = "MYSQL_AUDIT"
	LogTypeRedis            LogType = "REDIS"
	LogTypeElasticsearch    LogType = "ELASTICSEARCH"
	LogTypeKibana           LogType = "KIBANA"
	LogTypeKafka            LogType = "KAFKA"
	LogTypeGreenPlum        LogType = "GREENPLUM"
	LogTypeGreenPlumOdyssey LogType = "GREEENPLUM_POOLER"
)

// LogsServiceTypeToLogType translates general service type to type specific to logs listing
func LogsServiceTypeToLogType(st logs.ServiceType) []LogType {
	switch st {
	case logs.ServiceTypePostgreSQL:
		return []LogType{LogTypePostgreSQL}
	case logs.ServiceTypePooler:
		return []LogType{LogTypeOdyssey, LogTypePGBouncer}
	case logs.ServiceTypeClickHouse:
		return []LogType{LogTypeClickHouse}
	case logs.ServiceTypeMongoD:
		return []LogType{LogTypeMongoD}
	case logs.ServiceTypeMongoS:
		return []LogType{LogTypeMongoS}
	case logs.ServiceTypeMongoCFG:
		return []LogType{LogTypeMongoCFG}
	case logs.ServiceTypeMongoDBAudit:
		return []LogType{LogTypeMongoDBAudit}
	case logs.ServiceTypeMySQLGeneral:
		return []LogType{LogTypeMySQLGeneral}
	case logs.ServiceTypeMySQLError:
		return []LogType{LogTypeMySQLError}
	case logs.ServiceTypeMySQLSlowQuery:
		return []LogType{LogTypeMySQLSlowQuery}
	case logs.ServiceTypeMySQLAudit:
		return []LogType{LogTypeMySQLAudit}
	case logs.ServiceTypeRedis:
		return []LogType{LogTypeRedis}
	case logs.ServiceTypeElasticSearch:
		return []LogType{LogTypeElasticsearch}
	case logs.ServiceTypeKibana:
		return []LogType{LogTypeKibana}
	case logs.ServiceTypeKafka:
		return []LogType{LogTypeKafka}
	case logs.ServiceTypeGreenplum:
		return []LogType{LogTypeGreenPlum}
	case logs.ServiceTypeGreenplumPooler:
		return []LogType{LogTypeGreenPlumOdyssey}
	case logs.ServiceTypeOpenSearch:
		return []LogType{LogTypeElasticsearch}
	case logs.ServiceTypeOpenSearchDashboards:
		return []LogType{LogTypeKibana}
	default:
		return []LogType{LogTypeUnknown}
	}
}
