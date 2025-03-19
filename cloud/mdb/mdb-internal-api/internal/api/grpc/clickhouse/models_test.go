package clickhouse

import (
	"testing"

	"github.com/stretchr/testify/require"
	"google.golang.org/genproto/protobuf/field_mask"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	chv1config "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1/config"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

func TestUserUpdateFromGRPC(t *testing.T) {
	tests := []struct {
		name        string
		updateMask  *field_mask.FieldMask
		inSettings  *chv1.UserSettings
		outSettings *chmodels.UserSettings
	}{
		{
			name: "nil user settings",
			updateMask: &field_mask.FieldMask{
				Paths: []string{
					"settings",
				}},
			inSettings:  nil,
			outSettings: &chmodels.UserSettings{},
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			request := &chv1.UpdateUserRequest{
				ClusterId:  "cid1",
				UserName:   "user1",
				UpdateMask: test.updateMask,
				Settings:   test.inSettings,
			}

			args, err := UserUpdateFromGRPC(request)
			require.NoError(t, err)
			require.NotNil(t, args)
			require.Equal(t, args.Settings, test.outSettings)
		})
	}
}

func TestClusterToGRPC(t *testing.T) {
	tests := []struct {
		name            string
		inDictionaries  []chmodels.Dictionary
		outDictionaries []*chv1config.ClickhouseConfig_ExternalDictionary
	}{
		{
			name: "Cluster with PostgreSQL dictionary",
			inDictionaries: []chmodels.Dictionary{
				{
					Name: "dictionary1",
					PostgreSQLSource: chmodels.DictionarySourcePostgreSQL{
						DB:    "db1",
						Valid: true,
					},
				},
			},
			outDictionaries: []*chv1config.ClickhouseConfig_ExternalDictionary{
				{
					Name: "dictionary1",
					Source: &chv1config.ClickhouseConfig_ExternalDictionary_PostgresqlSource_{
						PostgresqlSource: &chv1config.ClickhouseConfig_ExternalDictionary_PostgresqlSource{
							Db: "db1",
						},
					},
					Structure: &chv1config.ClickhouseConfig_ExternalDictionary_Structure{
						Attributes: []*chv1config.ClickhouseConfig_ExternalDictionary_Structure_Attribute{},
					},
					Layout: &chv1config.ClickhouseConfig_ExternalDictionary_Layout{},
				},
			},
		},
	}

	clusterID := "cid1"
	version := "21.8"

	saltEnvMapper := grpc.NewSaltEnvMapper(
		int64(chv1.Cluster_PRODUCTION),
		int64(chv1.Cluster_PRESTABLE),
		int64(chv1.Cluster_ENVIRONMENT_UNSPECIFIED),
		logic.SaltEnvsConfig{
			Production: environment.SaltEnvProd,
			Prestable:  environment.SaltEnvQA,
		})

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			cluster := chmodels.MDBCluster{
				Cluster: chmodels.Cluster{
					ClusterExtended: clusters.ClusterExtended{
						Cluster: clusters.Cluster{
							ClusterID: clusterID,
						},
					},
				},
				Config: chmodels.ClusterConfig{
					Version: version,
					ClickhouseConfigSet: chmodels.ClickhouseConfigSet{
						Effective: chmodels.ClickHouseConfig{
							Dictionaries: test.inDictionaries,
						},
					},
				},
			}

			result := ClusterToGRPC(cluster, saltEnvMapper)
			require.NotNil(t, result)
			require.Equal(t, result.Id, clusterID)
			require.Equal(t, result.Config.Version, version)
			require.Equal(t, result.Config.Clickhouse.Config.EffectiveConfig.Dictionaries, test.outDictionaries)
		})
	}
}
