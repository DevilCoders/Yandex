package dbsupport

import (
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/mongodb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/mysql"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/postgresql"
	redisdb "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/redis"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/sqlserver"
)

var (
	DBspec = initDBspec()
	DBsupp = initDBsupp()
)

func initDBspec() map[metadb.ClusterType]dbspecific.Extractor {
	res := make(map[metadb.ClusterType]dbspecific.Extractor)
	res[metadb.ClickhouseCluster] = clickhouse.New()
	res[metadb.MongodbCluster] = mongodb.New()
	res[metadb.MysqlCluster] = mysql.New()
	res[metadb.RedisCluster] = redisdb.New()
	res[metadb.PostgresqlCluster] = postgresql.New()
	res[metadb.ElasticSearchCluster] = elasticsearch.New()
	res[metadb.KafkaCluster] = kafka.New()
	res[metadb.SQLServerCluster] = sqlserver.New()
	res[metadb.GreenplumCluster] = greenplum.New()

	return res
}

func initDBsupp() []metadb.ClusterType {
	res := make([]metadb.ClusterType, len(DBspec))
	i := 0
	for k := range DBspec {
		res[i] = k
		i++
	}

	return res
}
