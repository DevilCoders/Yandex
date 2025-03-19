package provider

import (
	"context"
	"encoding/json"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

func TestKafka_initResourcesIfEmpty(t *testing.T) {
	kafkaFixture := newKafkaFixture(t)

	t.Run("When resources are present should do nothing", func(t *testing.T) {
		resources := models.ClusterResources{
			ResourcePresetExtID: "ResourcePresetExtID",
			DiskSize:            1,
			DiskTypeExtID:       "DiskTypeExtID",
		}

		err := kafkaFixture.Kafka.initResourcesIfEmpty(&resources, hosts.RoleKafka)

		require.NoError(t, err)
		require.Equal(t, models.ClusterResources{
			ResourcePresetExtID: "ResourcePresetExtID",
			DiskSize:            1,
			DiskTypeExtID:       "DiskTypeExtID",
		}, resources)
	})

	t.Run("When resources are absent and no default resources in config should return error", func(t *testing.T) {
		kafkaFixture.Kafka.cfg.DefaultResources = logic.DefaultResourcesConfig{}
		resources := models.ClusterResources{}

		err := kafkaFixture.Kafka.initResourcesIfEmpty(&resources, hosts.RoleKafka)

		require.EqualError(t, err, "failed to get default resources for Kafka host with role Kafka: "+
			"unknown default resources for cluster type : Kafka")
	})

	t.Run("When resources are absent should fill from config", func(t *testing.T) {
		kafkaFixture.Kafka.cfg.DefaultResources = logic.DefaultResourcesConfig{
			ByClusterType: map[string]map[string]logic.DefaultResourcesRecord{
				clusters.TypeKafka.Stringified(): {
					hosts.RoleKafka.Stringified(): logic.DefaultResourcesRecord{
						ResourcePresetExtID: "ResourcePresetExtID",
						DiskSize:            1,
						DiskTypeExtID:       "DiskTypeExtID",
					},
				},
			},
		}
		resources := models.ClusterResources{}

		err := kafkaFixture.Kafka.initResourcesIfEmpty(&resources, hosts.RoleKafka)

		require.NoError(t, err)
		require.Equal(t, models.ClusterResources{
			ResourcePresetExtID: "ResourcePresetExtID",
			DiskSize:            1,
			DiskTypeExtID:       "DiskTypeExtID",
		}, resources)
	})
}

func TestKafka_DataCloudCluster(t *testing.T) {
	kafkaFixture := newKafkaFixture(t)
	mockOperatorReadCluster(kafkaFixture)
	adminUserName := "owner"
	dataCloudAdminPasswordPath := []string{"data", "kafka", "users", adminUserName, "password"}

	t.Run("When cluster can't be returned should return error", func(t *testing.T) {
		kafkaFixture.Reader.EXPECT().ClusterExtendedByClusterID(kafkaFixture.Context, "cid", clusters.TypeKafka, models.VisibilityVisible, kafkaFixture.Session).
			Return(clusters.ClusterExtended{}, semerr.InvalidInput("Can't get cluster")).
			Times(1)
		_, err := kafkaFixture.Kafka.DataCloudCluster(kafkaFixture.Context, "cid", false)
		require.EqualError(t, err, "Can't get cluster")
	})

	t.Run("When no owner user should return error", func(t *testing.T) {
		mockReaderClusterExtendedByClusterID(kafkaFixture, "cid", "{}")
		_, err := kafkaFixture.Kafka.DataCloudCluster(kafkaFixture.Context, "cid", false)
		require.EqualError(t, err, "can't find user: owner")
	})

	t.Run("Check maximum valid cluster state", func(t *testing.T) {
		const pillarRaw = `
	{
		"data": {
			"kafka": {
				"users": {
					"owner": {
						"password": {
							"data": "somePassword",
							"encryption_version": 0
						}
					}
				}
			}
		}
	}`
		mockReaderClusterExtendedByClusterID(kafkaFixture, "cid", pillarRaw)

		res, err := kafkaFixture.Kafka.DataCloudCluster(kafkaFixture.Context, "cid", false)

		require.NoError(t, err)

		expected := kfmodels.DataCloudCluster{
			Resources: kfmodels.DataCloudResources{
				ResourcePresetID: optional.NewString(""),
				BrokerCount:      optional.NewInt64(0),
				ZoneCount:        optional.NewInt64(0),
				DiskSize:         optional.NewInt64(0),
			},
		}
		expected.ClusterID = "cid"
		expected.Pillar = json.RawMessage(pillarRaw)

		require.Equal(t, expected, res)
	})

	t.Run("Check minimal valid cluster state without sensitive data", func(t *testing.T) {
		mockReaderClusterExtendedByClusterID(kafkaFixture, "cid", validPillar())

		res, err := kafkaFixture.Kafka.DataCloudCluster(kafkaFixture.Context, "cid", false)

		require.NoError(t, err)
		require.Equal(t, validDataCloudCluster(""), res)
	})

	t.Run("Check minimal valid cluster state with sensitive data", func(t *testing.T) {
		mockReaderClusterExtendedByClusterID(kafkaFixture, "cid", validPillar())
		kafkaFixture.PillarSecrets.EXPECT().
			GetClusterPillarSecret(kafkaFixture.Context, "cid", dataCloudAdminPasswordPath).
			Return(secret.NewString("newPasswordSecrets"), nil).
			Times(1)

		res, err := kafkaFixture.Kafka.DataCloudCluster(kafkaFixture.Context, "cid", true)

		require.NoError(t, err)
		require.Equal(t, validDataCloudCluster("newPasswordSecrets"), res)
	})
}

func mockReaderClusterExtendedByClusterID(kafkaFixture kafkaFixture, cid string, pillarRaw string) {
	cluster := clusters.ClusterExtended{
		Pillar: json.RawMessage(pillarRaw),
	}
	cluster.ClusterID = cid

	kafkaFixture.Reader.EXPECT().ClusterExtendedByClusterID(kafkaFixture.Context, "cid", clusters.TypeKafka, models.VisibilityVisible, kafkaFixture.Session).
		Return(cluster, nil).
		Times(1)
}

func mockOperatorReadCluster(kafkaFixture kafkaFixture) {
	kafkaFixture.Operator.EXPECT().ReadCluster(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		DoAndReturn(
			func(ctx context.Context, cid string, do clusterslogic.ReadClusterFunc, opts ...clusterslogic.OperatorOption) error {
				return do(ctx, kafkaFixture.Session, kafkaFixture.Reader)
			}).
		AnyTimes()
}

func validPillar() string {
	return `
	{
		"data": {
			"cloud_type": "aws",
			"region_id": "eu-central-1",
			"kafka": {
				"version": "2.8",
				"zone_id": ["zoneId1", "zoneId2", "zoneId3"],
				"brokers_count": 1,
				"resources": {
					"ResourcePresetExtID": "flavorId1",
					"DiskSize": 2000
				},
				"nodes": {
					"host1.yadc.io": {
						"fqdn": "host1.yadc.io",
						"private_fqdn": "host1.private.yadc.io",
						"rack": "zone1"
					},
					"host2.yadc.io": {
						"fqdn": "host2.yadc.io",
						"private_fqdn": "host2.private.yadc.io",
						"rack": "zone2"
					},
					"host3.yadc.io": {
						"fqdn": "host3.yadc.io",
						"private_fqdn": "host3.private.yadc.io",
						"rack": "zone3"
					}
				},
				"users": {
					"owner": {
						"name": "ownerName",
						"password": {
							"data": "somePassword",
							"encryption_version": 0
						}
					}
				}
			},
			"access": {
				"data_transfer": true,
				"ipv4_cidr_blocks": [
					{
						"value": "0.0.0.0/0",
						"description": "ipv4 first"
					},
					{
						"value": "192.168.1.10/32",
						"description": "ipv4 second"
					}
				],
				"ipv6_cidr_blocks": [
					{
						"value": "::/0",
						"description": "ipv6 first"
					},
					{
						"value": "::/1",
						"description": "ipv6 second"
					}
				]
			},
			"encryption": {
				"enabled": true
			}
		}
	}`
}

func validDataCloudCluster(connectionPassword string) kfmodels.DataCloudCluster {
	expected := kfmodels.DataCloudCluster{
		CloudType: environment.CloudTypeAWS,
		Version:   "2.8",
		RegionID:  "eu-central-1",
		ConnectionInfo: kfmodels.ConnectionInfo{
			ConnectionString: "host1.yadc.io:9091,host2.yadc.io:9091",
			User:             "ownerName",
			Password:         secret.NewString(connectionPassword),
		},
		PrivateConnectionInfo: kfmodels.PrivateConnectionInfo{
			ConnectionString: "host1.private.yadc.io:19091,host2.private.yadc.io:19091",
			User:             "ownerName",
			Password:         secret.NewString(connectionPassword),
		},
		Resources: kfmodels.DataCloudResources{
			ResourcePresetID: optional.NewString("flavorId1"),
			BrokerCount:      optional.NewInt64(1),
			ZoneCount:        optional.NewInt64(3),
			DiskSize:         optional.NewInt64(2000),
		},
		Access: clusters.Access{
			Ipv4CidrBlocks: []clusters.CidrBlock{
				{
					Value:       "0.0.0.0/0",
					Description: "ipv4 first",
				},
				{
					Value:       "192.168.1.10/32",
					Description: "ipv4 second",
				},
			},
			Ipv6CidrBlocks: []clusters.CidrBlock{
				{
					Value:       "::/0",
					Description: "ipv6 first",
				},
				{
					Value:       "::/1",
					Description: "ipv6 second",
				},
			},
			DataTransfer: optional.NewBool(true),
		},
		Encryption: clusters.Encryption{
			Enabled: optional.NewBool(true),
		},
	}
	expected.ClusterID = "cid"
	expected.Pillar = json.RawMessage(validPillar())

	return expected
}
