package console

import (
	"golang.org/x/xerrors"

	consolev1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

func clusterTypeFromModelToGRPC(modelType clusters.Type) (consolev1.ClusterType, error) {
	switch modelType {
	case clusters.TypeClickHouse:
		return consolev1.ClusterType_CLICKHOUSE, nil
	case clusters.TypePostgreSQL:
		return consolev1.ClusterType_POSTGRESQL, nil
	case clusters.TypeMongoDB:
		return consolev1.ClusterType_MONGODB, nil
	case clusters.TypeRedis:
		return consolev1.ClusterType_REDIS, nil
	case clusters.TypeMySQL:
		return consolev1.ClusterType_MYSQL, nil
	case clusters.TypeKafka:
		return consolev1.ClusterType_KAFKA, nil
	case clusters.TypeElasticSearch:
		return consolev1.ClusterType_ELASTICSEARCH, nil
	case clusters.TypeOpenSearch:
		return consolev1.ClusterType_OPENSEARCH, nil
	case clusters.TypeSQLServer:
		return consolev1.ClusterType_SQLSERVER, nil
	case clusters.TypeHadoop:
		return consolev1.ClusterType_HADOOP, nil
	case clusters.TypeGreenplumCluster:
		return consolev1.ClusterType_GREENPLUM, nil
	default:
		return consolev1.ClusterType_CLUSTER_TYPE_UNSPECIFIED, xerrors.Errorf("unknown cluster type: %s", modelType)
	}
}
