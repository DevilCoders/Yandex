package console_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	consolev1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/console"
	grpcconsole "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/console"
	_ "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console/mocks"
	_ "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	_ "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

func TestCUDService_GetBillingMetrics(t *testing.T) {
	tests := []struct {
		inClusterType  consolev1.GetBillingMetricsRequest_Type
		outClusterType string
		outRole        string
	}{
		{
			inClusterType:  consolev1.GetBillingMetricsRequest_POSTGRESQL,
			outClusterType: clusters.TypePostgreSQL.Stringified(),
			outRole:        hosts.RolePostgreSQL.Stringified(),
		},
		{
			inClusterType:  consolev1.GetBillingMetricsRequest_CLICKHOUSE,
			outClusterType: clusters.TypeClickHouse.Stringified(),
			outRole:        hosts.RoleClickHouse.Stringified(),
		},
		{
			inClusterType:  consolev1.GetBillingMetricsRequest_MONGODB,
			outClusterType: clusters.TypeMongoDB.Stringified(),
			outRole:        hosts.RoleMongoD.Stringified(),
		},
		{
			inClusterType:  consolev1.GetBillingMetricsRequest_REDIS,
			outClusterType: clusters.TypeRedis.Stringified(),
			outRole:        hosts.RoleRedis.Stringified(),
		},
		{
			inClusterType:  consolev1.GetBillingMetricsRequest_MYSQL,
			outClusterType: clusters.TypeMySQL.Stringified(),
			outRole:        hosts.RoleMySQL.Stringified(),
		},
		{
			inClusterType:  consolev1.GetBillingMetricsRequest_KAFKA,
			outClusterType: clusters.TypeKafka.Stringified(),
			outRole:        hosts.RoleKafka.Stringified(),
		},
		{
			inClusterType:  consolev1.GetBillingMetricsRequest_ELASTICSEARCH,
			outClusterType: clusters.TypeElasticSearch.Stringified(),
			outRole:        hosts.RoleElasticSearchDataNode.Stringified(),
		},
	}

	ctx := context.Background()

	for _, test := range tests {
		t.Run(string(test.outClusterType), func(t *testing.T) {
			ctrl := gomock.NewController(t)
			console := mocks.NewMockConsole(ctrl)
			cs := &grpcconsole.CUDService{Console: console}

			resp, err := cs.GetBillingMetrics(ctx,
				&consolev1.GetBillingMetricsRequest{
					ClusterType: test.inClusterType,
					PlatformId:  "mdb-v1",
					Cores:       1,
					Memory:      655360,
					Edition:     "basic",
				},
			)

			require.NoError(t, err)
			require.NotNil(t, resp)

			require.Equal(t, consolemodels.BillingSchemaNameYandexCloud, resp.Schema)
			require.Equal(t, "mdb-v1", resp.Tags.PlatformId)
			require.Equal(t, "basic", resp.Tags.Edition)
			require.Equal(t, int64(1), resp.Tags.Cores)
			require.Equal(t, int64(100), resp.Tags.CoreFraction)
			require.Equal(t, int64(655360), resp.Tags.Memory)

			require.Equal(t, string(test.outClusterType), resp.Tags.ClusterType)
			require.Equal(t, string(test.outRole), resp.Tags.Roles[0])
		})
	}
}
