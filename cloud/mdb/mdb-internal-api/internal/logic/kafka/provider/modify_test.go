package provider

import (
	"context"
	"encoding/json"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/featureflags"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	clustersmocks "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	internaltasks "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

func TestUpgradeNotImplementedWhenFeatureFlagKafkaAllowUpgradeIsNotSetUp(t *testing.T) {
	pillar := kfpillars.NewCluster()
	pillar.Data.Kafka.Version = "2.6"

	cluster := clusterslogic.NewClusterModel(clusters.Cluster{
		ClusterID:   clusterID,
		Revision:    revisionID,
		Environment: environment.SaltEnvDev,
	}, marshalPillar(t, pillar))

	kafkaFixture := newKafkaFixture(t)
	mockOperatorModifyOnCluster(kafkaFixture, cluster)
	mockModifierModifyClusterMetadata(kafkaFixture, cluster, false)
	mockModifierModifyClusterMetadataParameters(kafkaFixture, cluster, false)

	args := kafka.ModifyMDBClusterArgs{
		ClusterID: clusterID,
		ConfigSpec: kfmodels.ClusterConfigSpecMDBUpdate{
			Version: optional.NewString("2.8"),
		},
	}
	_, err := kafkaFixture.Kafka.ModifyMDBCluster(kafkaFixture.Context, args)
	require.EqualError(t, err, "changing version of the cluster is not implemented")
}

func TestUpgradeToVersion32ShouldFail(t *testing.T) {
	pillar := kfpillars.NewCluster()
	pillar.Data.Kafka.Version = "3.0"

	cluster := clusterslogic.NewClusterModel(clusters.Cluster{
		ClusterID:   clusterID,
		Revision:    revisionID,
		Environment: environment.SaltEnvDev,
	}, marshalPillar(t, pillar))

	kafkaFixture := newKafkaFixture(t)
	kafkaFixture.Session.FeatureFlags = featureflags.NewFeatureFlags([]string{kfmodels.KafkaAllowUpgradeFeatureFlag})
	mockOperatorModifyOnCluster(kafkaFixture, cluster)

	args := kafka.ModifyMDBClusterArgs{
		ClusterID: clusterID,
		ConfigSpec: kfmodels.ClusterConfigSpecMDBUpdate{
			Version: optional.NewString("3.2"),
		},
	}
	_, err := kafkaFixture.Kafka.ModifyMDBCluster(kafkaFixture.Context, args)
	require.EqualError(t, err, "update to version 3.2 is not supported yet")
}

func TestUpgradeSuccessWhenFeatureFlagKafkaAlloUpgradeIsSetUp(t *testing.T) {
	pillar := kfpillars.NewCluster()
	pillar.Data.Kafka.Version = "2.6"

	expectedPillar := copyPillar(t, pillar)
	expectedPillar.Data.Kafka.Version = "2.8"
	expectedPillar.Data.Kafka.InterBrokerProtocolVersion = "2.6"
	expectedPillar.Data.Kafka.PackageVersion = "2.8.1.1-java11"

	cluster := clusterslogic.NewClusterModel(clusters.Cluster{
		ClusterID:   clusterID,
		Revision:    revisionID,
		Environment: environment.SaltEnvDev,
	}, marshalPillar(t, pillar))

	kafkaFixture := newKafkaFixture(t)
	kafkaFixture.Session.FeatureFlags = featureflags.NewFeatureFlags([]string{kfmodels.KafkaAllowUpgradeFeatureFlag})
	mockOperatorModifyOnCluster(kafkaFixture, cluster)
	mockModifierModifyClusterMetadata(kafkaFixture, cluster, false)
	mockModifierModifyClusterMetadataParameters(kafkaFixture, cluster, false)
	assertModifierUpdatePillar(kafkaFixture, *expectedPillar)
	op := operations.Operation{}
	mockTasksUpgradeCluster(kafkaFixture, clusterID, revisionID, op)

	args := kafka.ModifyMDBClusterArgs{
		ClusterID: clusterID,
		ConfigSpec: kfmodels.ClusterConfigSpecMDBUpdate{
			Version: optional.NewString("2.8"),
		},
	}
	resultOp, err := kafkaFixture.Kafka.ModifyMDBCluster(kafkaFixture.Context, args)
	require.NoError(t, err)
	require.Equal(t, op, resultOp)
}

func TestUpgradeCannotMix(t *testing.T) {
	pillar := kfpillars.NewCluster()
	pillar.Data.Kafka.Version = "2.6"
	cluster := clusterslogic.NewClusterModel(clusters.Cluster{
		ClusterID:   clusterID,
		Revision:    revisionID,
		Environment: environment.SaltEnvDev,
	}, marshalPillar(t, pillar))

	kafkaFixture := newKafkaFixture(t)
	kafkaFixture.Session.FeatureFlags = featureflags.NewFeatureFlags([]string{kfmodels.KafkaAllowUpgradeFeatureFlag})
	mockOperatorModifyOnCluster(kafkaFixture, cluster)
	mockModifierModifyClusterMetadata(kafkaFixture, cluster, false)
	mockModifierModifyClusterMetadataParameters(kafkaFixture, cluster, false)

	logSegmentSize := int64(100 * 1024 * 1024)
	args := kafka.ModifyMDBClusterArgs{
		ClusterID: clusterID,
		ConfigSpec: kfmodels.ClusterConfigSpecMDBUpdate{
			Version: optional.NewString("2.8"),
			Kafka: kfmodels.KafkaConfigSpecUpdate{
				Config: kfmodels.KafkaConfigUpdate{
					LogSegmentBytes: optional.NewInt64Pointer(&logSegmentSize),
				},
			},
		},
	}
	_, err := kafkaFixture.Kafka.ModifyMDBCluster(kafkaFixture.Context, args)
	require.EqualError(t, err, "Version update cannot be mixed with any other changes")
}

func TestUpgradeZookeeperResources(t *testing.T) {
	t.Run("When pillar doesn't contain information about zk cluster and modify zk resources should return error", func(t *testing.T) {
		kafkaFixture := newKafkaFixture(t)
		cluster := clusterslogic.NewClusterModel(clusters.Cluster{
			Environment: environment.SaltEnvDev,
		}, json.RawMessage(`{"data": {"kafka": {}}}`))
		mockOperatorModifyOnCluster(kafkaFixture, cluster)

		_, err := kafkaFixture.Kafka.ModifyMDBCluster(kafkaFixture.Context, kafka.ModifyMDBClusterArgs{
			ClusterID: clusterID,
			ConfigSpec: kfmodels.ClusterConfigSpecMDBUpdate{
				ZooKeeper: kfmodels.ZookeeperConfigSpecUpdate{
					Resources: models.ClusterResourcesSpec{
						DiskSize: optional.NewInt64(zkDiskSize),
					},
				},
			},
		})
		require.EqualError(t, err, "cannot change zookeeper resources: this cluster doesn't have dedicated zookeeper hosts")
	})

	t.Run("Increase disk size for ZK cluster", func(t *testing.T) {
		kafkaFixture := newKafkaFixture(t)
		cluster := clusterslogic.NewClusterModel(clusters.Cluster{
			ClusterID:   clusterID,
			Revision:    revisionID,
			Environment: environment.SaltEnvDev,
		}, json.RawMessage(`{"data":{"kafka": {"has_zk_subcluster": true}}}`))
		mockOperatorModifyOnCluster(kafkaFixture, cluster)
		mockModifierModifyClusterMetadata(kafkaFixture, cluster, false)
		mockModifierModifyClusterMetadataParameters(kafkaFixture, cluster, false)
		mockReaderListHostsFor2KafkaAnd3ZK(kafkaFixture)
		newZkDiskSize := int64(1000000)
		assertModifierValidateResourcesForZkIncreaseDiskSize(kafkaFixture, newZkDiskSize)
		zkModifyArgsTemplate := models.ModifyHostArgs{
			ClusterID:        clusterID,
			Revision:         cluster.Revision,
			SpaceLimit:       newZkDiskSize,
			ResourcePresetID: zkPresetID,
			DiskTypeExtID:    zkDiskTypeExtID,
		}
		zk1ModifyArgs := zkModifyArgsTemplate
		zk1ModifyArgs.FQDN = zkFQDN1
		zk2ModifyArgs := zkModifyArgsTemplate
		zk2ModifyArgs.FQDN = zkFQDN2
		zk3ModifyArgs := zkModifyArgsTemplate
		zk3ModifyArgs.FQDN = zkFQDN3
		assertModifierModifyHost(kafkaFixture, zk1ModifyArgs, nil)
		assertModifierModifyHost(kafkaFixture, zk2ModifyArgs, nil)
		assertModifierModifyHost(kafkaFixture, zk3ModifyArgs, nil)
		expectedPillar := minimalValidPillar()
		expectedPillar.Data.Kafka.HasZkSubcluster = true
		expectedPillar.Data.ZooKeeper.Resources.DiskSize = newZkDiskSize
		assertModifierUpdatePillar(kafkaFixture, expectedPillar)
		assertModifyCluster(kafkaFixture, optional.Strings{}, internaltasks.ModifyClusterOptions{
			TaskArgs: map[string]interface{}{
				"restart": false,
			},
			Timeout: optional.NewDuration(60 * time.Minute),
		}, resultOperation)

		actualOperation, err := kafkaFixture.Kafka.ModifyMDBCluster(kafkaFixture.Context, kafka.ModifyMDBClusterArgs{
			ClusterID: clusterID,
			ConfigSpec: kfmodels.ClusterConfigSpecMDBUpdate{
				ZooKeeper: kfmodels.ZookeeperConfigSpecUpdate{
					Resources: models.ClusterResourcesSpec{
						DiskSize: optional.NewInt64(newZkDiskSize),
					},
				},
			},
		})
		require.NoError(t, err)
		require.Equal(t, resultOperation, actualOperation)
	})
}

func TestSubtractElements(t *testing.T) {
	require.Equal(t, []string(nil), subtractElements([]string{}, []string{}))
	require.Equal(t, []string{"a", "b", "c"}, subtractElements([]string{"a", "b", "c"}, []string{}))
	require.Equal(t, []string(nil), subtractElements([]string{}, []string{"a", "b", "c"}))
	require.Equal(t, []string{"a", "c"}, subtractElements([]string{"a", "b", "c"}, []string{"b"}))
	require.Equal(t, []string(nil), subtractElements([]string{"a", "b", "c"}, []string{"a", "b", "c"}))
	require.Equal(t, []string{"a", "b", "c"}, subtractElements([]string{"a", "b", "c"}, []string{"d", "e", "f"}))
	require.Equal(t, []string{"a"}, subtractElements([]string{"a", "b", "c"}, []string{"b", "c", "d"}))
}

func TestGetUpdatedDataCloudKafkaZones(t *testing.T) {
	ctx := context.Background()
	session := sessions.Session{}
	ctrl := gomock.NewController(t)
	modifier := clustersmocks.NewMockModifier(ctrl)
	pillar := kfpillars.NewCluster()
	pillar.Data.Kafka.ZoneID = []string{zoneA, zoneB}
	pillar.Data.RegionID = regionID
	pillar.Data.CloudType = environment.CloudTypeAWS

	t.Run("When zone count is empty should return empty kafka zones", func(t *testing.T) {
		result, err := getUpdatedDataCloudKafkaZones(ctx, session, modifier, kfpillars.NewCluster(), optional.Int64{}, optional.Int64{})

		require.NoError(t, err)
		require.Equal(t, optional.Strings{}, result)
	})

	t.Run("When new zone count less than current number of zones should return error", func(t *testing.T) {
		_, err := getUpdatedDataCloudKafkaZones(ctx, session, modifier, pillar, optional.NewInt64(1), optional.NewInt64(1))

		require.EqualError(t, err, "removing zones is not implemented")
	})

	t.Run("When zone counts equal to current zone counts should return empty", func(t *testing.T) {
		result, err := getUpdatedDataCloudKafkaZones(ctx, session, modifier, pillar, optional.NewInt64(2), optional.NewInt64(1))

		require.NoError(t, err)
		require.Equal(t, optional.Strings{}, result)
	})

	t.Run("When was two zones in pillar and need to update to 3 should return two old and 1 new zones", func(t *testing.T) {
		modifier.EXPECT().SelectZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, regionID, true, 3).
			Return(zonesABC, nil)

		result, err := getUpdatedDataCloudKafkaZones(ctx, session, modifier, pillar, optional.NewInt64(3), optional.NewInt64(1))

		require.NoError(t, err)
		require.Equal(t, optional.Strings{Strings: zonesABC, Valid: true}, result)
	})

	t.Run("When was two zones in pillar and need to update to 4 and zones are different from pillar should add 2 new zone", func(t *testing.T) {
		modifier.EXPECT().SelectZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, regionID, true, 4).
			Return([]string{zoneB, zoneC, "us-east-1d", "us-east-1e"}, nil)

		result, err := getUpdatedDataCloudKafkaZones(ctx, session, modifier, pillar, optional.NewInt64(4), optional.NewInt64(1))

		require.NoError(t, err)
		require.Equal(t, optional.Strings{Strings: []string{zoneA, zoneB, zoneC, "us-east-1d"}, Valid: true}, result)
	})
}

func TestConvertModifyDataCloudArgs(t *testing.T) {
	ctx := context.Background()
	session := sessions.Session{}
	ctrl := gomock.NewController(t)
	modifier := clustersmocks.NewMockModifier(ctrl)
	pillar := kfpillars.NewCluster()
	pillar.Data.Kafka.ZoneID = []string{"us-east-1a", "us-east-1b"}
	pillar.Data.RegionID = "us-east-1"
	pillar.Data.CloudType = environment.CloudTypeAWS

	t.Run("When try to reduce zone should return error", func(t *testing.T) {
		args := kafka.ModifyDataCloudClusterArgs{ConfigSpec: kfmodels.ClusterConfigSpecDataCloudUpdate{ZoneCount: optional.NewInt64(1)}}
		_, err := convertModifyDataCloudArgs(ctx, session, modifier, pillar, args, true)
		require.EqualError(t, err, "removing zones is not implemented")
	})

	t.Run("When zone count is the same and parameters are absent should return empty structure", func(t *testing.T) {
		args := kafka.ModifyDataCloudClusterArgs{ConfigSpec: kfmodels.ClusterConfigSpecDataCloudUpdate{ZoneCount: optional.NewInt64(2)}}
		result, err := convertModifyDataCloudArgs(ctx, session, modifier, pillar, args, false)
		require.NoError(t, err)
		require.Equal(t, modifyClusterImplArgs{}, result)
	})

	t.Run("Check setting all fields", func(t *testing.T) {
		modifier.EXPECT().SelectZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, "us-east-1", true, 3).
			Return([]string{"us-east-1b", "us-east-1c", "us-east-1d"}, nil)
		kafkaConfigSpec := kfmodels.KafkaConfigSpecUpdate{
			Config: kfmodels.KafkaConfigUpdate{
				CompressionType: optional.NewString("compressionTYpe"),
			},
		}
		args := kafka.ModifyDataCloudClusterArgs{
			ClusterID:   "clusterId",
			Name:        optional.NewString("name"),
			Description: optional.NewString("description"),
			Labels:      modelsoptional.NewLabels(map[string]string{"labelKey1": "labelValue1"}),
			ConfigSpec: kfmodels.ClusterConfigSpecDataCloudUpdate{
				ZoneCount:    optional.NewInt64(3),
				BrokersCount: optional.NewInt64(1),
				Version:      optional.NewString("2.8"),
				Kafka:        kafkaConfigSpec,
			},
		}

		result, err := convertModifyDataCloudArgs(ctx, session, modifier, pillar, args, true)

		require.NoError(t, err)
		require.Equal(t, modifyClusterImplArgs{
			ClusterID:   "clusterId",
			Name:        optional.NewString("name"),
			Description: optional.NewString("description"),
			Labels:      modelsoptional.NewLabels(map[string]string{"labelKey1": "labelValue1"}),
			ConfigSpec: configSpecUpdate{
				Version:      args.ConfigSpec.Version,
				Kafka:        kafkaConfigSpec,
				ZoneID:       optional.Strings{Strings: []string{"us-east-1a", "us-east-1b", "us-east-1c"}, Valid: true},
				BrokersCount: optional.NewInt64(1),
			},
			IsPrestable: true,
		}, result)
	})

	t.Run("Check setting all fields and zone count is the same as in pillar should return object with empty kafka zones", func(t *testing.T) {
		kafkaConfigSpec := kfmodels.KafkaConfigSpecUpdate{
			Config: kfmodels.KafkaConfigUpdate{
				CompressionType: optional.NewString("compressionTYpe"),
			},
		}
		args := kafka.ModifyDataCloudClusterArgs{
			ClusterID:   "clusterId",
			Name:        optional.NewString("name"),
			Description: optional.NewString("description"),
			Labels:      modelsoptional.NewLabels(map[string]string{"labelKey1": "labelValue1"}),
			ConfigSpec: kfmodels.ClusterConfigSpecDataCloudUpdate{
				ZoneCount:    optional.NewInt64(2),
				BrokersCount: optional.NewInt64(1),
				Version:      optional.NewString("2.8"),
				Kafka:        kafkaConfigSpec,
			},
		}

		result, err := convertModifyDataCloudArgs(ctx, session, modifier, pillar, args, true)

		require.NoError(t, err)
		require.Equal(t, modifyClusterImplArgs{
			ClusterID:   "clusterId",
			Name:        optional.NewString("name"),
			Description: optional.NewString("description"),
			Labels:      modelsoptional.NewLabels(map[string]string{"labelKey1": "labelValue1"}),
			ConfigSpec: configSpecUpdate{
				Version:      args.ConfigSpec.Version,
				Kafka:        kafkaConfigSpec,
				ZoneID:       optional.Strings{},
				BrokersCount: optional.NewInt64(1),
			},
			IsPrestable: true,
		}, result)
	})
}

func TestModifyDataCloudCluster(t *testing.T) {
	t.Run("Minimal setup for calling modify without any changes", func(t *testing.T) {
		kafkaFixture := newKafkaFixture(t)
		cluster := clusterslogic.NewClusterModel(clusters.Cluster{
			Environment: environment.SaltEnvDev,
		}, json.RawMessage(`{"data": {"kafka": {}}}`))

		mockOperatorModifyOnCluster(kafkaFixture, cluster)
		mockModifierModifyClusterMetadata(kafkaFixture, cluster, false)
		mockModifierModifyClusterMetadataParameters(kafkaFixture, cluster, false)

		_, err := kafkaFixture.Kafka.ModifyDataCloudCluster(kafkaFixture.Context, kafka.ModifyDataCloudClusterArgs{ClusterID: clusterID})
		require.EqualError(t, err, "no changes detected")
	})

	t.Run("Minimal setup for calling modify with updating metadb only", func(t *testing.T) {
		kafkaFixture := newKafkaFixture(t)
		cluster := clusterslogic.NewClusterModel(clusters.Cluster{
			ClusterID:   clusterID,
			Revision:    revisionID,
			Environment: environment.SaltEnvDev,
		}, json.RawMessage(`{"data": {"kafka": {}}}`))

		mockOperatorModifyOnCluster(kafkaFixture, cluster)
		mockModifierModifyClusterMetadata(kafkaFixture, cluster, false)
		mockModifierModifyClusterMetadataParameters(kafkaFixture, cluster, true)

		assertCreateFinishedTask(kafkaFixture, resultOperation)
		assertSearchStoreDoc(kafkaFixture, resultOperation)

		actualOperation, err := kafkaFixture.Kafka.ModifyDataCloudCluster(kafkaFixture.Context, kafka.ModifyDataCloudClusterArgs{ClusterID: clusterID})

		require.NoError(t, err)
		require.Equal(t, resultOperation, actualOperation)
	})

	t.Run("Check adding access parameters", func(t *testing.T) {
		kafkaFixture := newKafkaFixture(t)
		args := kafka.ModifyDataCloudClusterArgs{
			ClusterID: clusterID,
			ConfigSpec: kfmodels.ClusterConfigSpecDataCloudUpdate{
				Access: validClustersAccess(),
			},
		}
		cluster := clusterslogic.NewClusterModel(clusters.Cluster{
			ClusterID:   clusterID,
			Revision:    revisionID,
			Environment: environment.SaltEnvDev,
		}, json.RawMessage(`{"data": {"kafka": {}}}`))

		mockOperatorModifyOnCluster(kafkaFixture, cluster)
		mockModifierModifyClusterMetadata(kafkaFixture, cluster, false)
		mockModifierModifyClusterMetadataParameters(kafkaFixture, cluster, false)

		expectedPillar := minimalValidPillar()
		expectedPillar.Data.Access = validAccessSettings()
		assertModifierUpdatePillar(kafkaFixture, expectedPillar)
		assertModifyCluster(kafkaFixture, optional.Strings{}, internaltasks.ModifyClusterOptions{
			TaskArgs: map[string]interface{}{
				"restart":         false,
				"update-firewall": true,
			},
			Timeout: optional.NewDuration(60 * time.Minute),
		}, resultOperation)

		actualOperation, err := kafkaFixture.Kafka.ModifyDataCloudCluster(kafkaFixture.Context, args)

		require.NoError(t, err)
		require.Equal(t, resultOperation, actualOperation)
	})
}
func assertModifierValidateResources(kafkaFixture kafkaFixture, expectedHostGroup clusterslogic.HostGroup,
	returnedResolvedHostGroups clusterslogic.ResolvedHostGroups) {
	kafkaFixture.Modifier.EXPECT().ValidateResources(
		kafkaFixture.Context,
		kafkaFixture.Session,
		clusters.TypeKafka,
		gomock.Any()).
		DoAndReturn(func(ctx context.Context, session sessions.Session, typ clusters.Type,
			hostGroups ...clusterslogic.HostGroup) (clusterslogic.ResolvedHostGroups, bool, error) {

			require.Equal(kafkaFixture.T, expectedHostGroup, hostGroups[0])
			return returnedResolvedHostGroups, true, nil
		})
}

func assertModifierValidateResourcesForZkIncreaseDiskSize(kafkaFixture kafkaFixture, newDiskSize int64) {
	assertModifierValidateResources(kafkaFixture, clusterslogic.HostGroup{
		Role:                       hosts.RoleZooKeeper,
		DiskTypeExtID:              zkDiskTypeExtID,
		HostsCurrent:               []clusterslogic.ZoneHosts{zoneHosts(zoneA, 1), zoneHosts(zoneB, 1), zoneHosts(zoneC, 1)},
		CurrentResourcePresetExtID: optional.NewString(zkResourcePresetExtID),
		CurrentDiskSize:            optional.NewInt64(zkDiskSize),
		NewDiskSize:                optional.NewInt64(newDiskSize),
		SkipValidations: clusterslogic.SkipValidations{
			DecommissionedZone: true,
		},
	}, clusterslogic.NewResolvedHostGroups([]clusterslogic.ResolvedHostGroup{
		{
			HostGroup: clusterslogic.HostGroup{
				Role:          hosts.RoleZooKeeper,
				NewDiskSize:   optional.NewInt64(newDiskSize),
				DiskTypeExtID: zkDiskTypeExtID,
			},
			CurrentResourcePreset: resources.Preset{
				ID:    zkPresetID,
				VType: environment.VTypePorto,
			},
		},
	}))
}

func zoneHosts(zoneID string, count int) clusterslogic.ZoneHosts {
	return clusterslogic.ZoneHosts{
		ZoneID: zoneID,
		Count:  int64(count),
	}
}

func mockReaderListHosts(kafkaFixture kafkaFixture, clusterID string, returnedHosts []hosts.HostExtended) {
	kafkaFixture.Reader.EXPECT().ListHosts(kafkaFixture.Context, clusterID, int64(0), int64(0)).
		Return(returnedHosts, int64(0), false, nil)
}

func mockReaderListHostsFor2KafkaAnd3ZK(kafkaFixture kafkaFixture) {
	mockReaderListHosts(kafkaFixture, clusterID, []hosts.HostExtended{
		{
			Host: hosts.Host{
				FQDN:                zkFQDN1,
				Roles:               []hosts.Role{hosts.RoleZooKeeper},
				ClusterID:           clusterID,
				SubnetID:            subnetIDA,
				SubClusterID:        zkSubClusterID,
				ResourcePresetExtID: zkResourcePresetExtID,
				DiskTypeExtID:       zkDiskTypeExtID,
				SpaceLimit:          zkDiskSize,
				ZoneID:              zoneA,
			},
		},
		{
			Host: hosts.Host{
				FQDN:                zkFQDN2,
				Roles:               []hosts.Role{hosts.RoleZooKeeper},
				ClusterID:           clusterID,
				SubnetID:            subnetIDB,
				SubClusterID:        zkSubClusterID,
				ResourcePresetExtID: zkResourcePresetExtID,
				DiskTypeExtID:       zkDiskTypeExtID,
				SpaceLimit:          zkDiskSize,
				ZoneID:              zoneB,
			},
		},
		{
			Host: hosts.Host{
				FQDN:                zkFQDN3,
				Roles:               []hosts.Role{hosts.RoleZooKeeper},
				ClusterID:           clusterID,
				SubnetID:            subnetIDC,
				SubClusterID:        zkSubClusterID,
				ResourcePresetExtID: zkResourcePresetExtID,
				DiskTypeExtID:       zkDiskTypeExtID,
				SpaceLimit:          zkDiskSize,
				ZoneID:              zoneC,
			},
		},
		{
			Host: hosts.Host{
				FQDN:         kafkaFQDN1,
				Roles:        []hosts.Role{hosts.RoleKafka},
				ClusterID:    clusterID,
				SubnetID:     subnetIDA,
				SubClusterID: kafkaSubClusterID,
			},
		},
		{
			Host: hosts.Host{
				FQDN:         kafkaFQDN2,
				Roles:        []hosts.Role{hosts.RoleKafka},
				ClusterID:    clusterID,
				SubnetID:     subnetIDB,
				SubClusterID: kafkaSubClusterID,
			},
		},
	})
}

func mockOperatorModifyOnCluster(kafkaFixture kafkaFixture, cluster clusterslogic.Cluster) {
	kafkaFixture.Operator.EXPECT().ModifyOnCluster(kafkaFixture.Context, clusterID, clusters.TypeKafka, gomock.Any(), gomock.Any()).
		DoAndReturn(func(ctx context.Context, cid string, typ clusters.Type, do clusterslogic.ModifyOnClusterFunc, opts ...clusterslogic.OperatorOption) (operations.Operation, error) {
			op, err := do(ctx, kafkaFixture.Session, kafkaFixture.Reader, kafkaFixture.Modifier, cluster)
			return op, err
		}).AnyTimes()
}

func mockModifierModifyClusterMetadata(kafkaFixture kafkaFixture, cluster clusterslogic.Cluster, returnedMetadataOnlyChanges bool) {
	mockModifierModifyClusterMetadataExt(kafkaFixture, cluster, optional.String{}, modelsoptional.Labels{}, returnedMetadataOnlyChanges, nil)
}

func mockModifierModifyClusterMetadataExt(kafkaFixture kafkaFixture, cluster clusterslogic.Cluster,
	name optional.String, labels modelsoptional.Labels, returnedMetadataOnlyChanges bool, returnedError error) {
	kafkaFixture.Modifier.EXPECT().ModifyClusterMetadata(kafkaFixture.Context, cluster, name, labels).
		Return(returnedMetadataOnlyChanges, returnedError)
}

func mockModifierModifyClusterMetadataParameters(kafkaFixture kafkaFixture, cluster clusterslogic.Cluster, returnedMetadbOnlyChanges bool) {
	mockModifierModifyClusterMetadataParametersExt(kafkaFixture, cluster, optional.String{}, modelsoptional.Labels{},
		optional.Bool{}, modelsoptional.MaintenanceWindow{}, returnedMetadbOnlyChanges, nil)
}

func mockModifierModifyClusterMetadataParametersExt(kafkaFixture kafkaFixture, cluster clusterslogic.Cluster,
	description optional.String, labels modelsoptional.Labels, deletionProtection optional.Bool, maintenanceWindow modelsoptional.MaintenanceWindow,
	returnedMetadbOnlyChanges bool, returnedError error) {
	kafkaFixture.Modifier.EXPECT().ModifyClusterMetadataParameters(kafkaFixture.Context, cluster, description, labels, deletionProtection, maintenanceWindow).
		Return(returnedMetadbOnlyChanges, returnedError)
}

func mockTasksUpgradeCluster(kafkaFixture kafkaFixture, clusterID string, revisionID int64, returnedOperation operations.Operation) {
	kafkaFixture.Tasks.EXPECT().UpgradeCluster(kafkaFixture.Context, kafkaFixture.Session, clusterID, revisionID, kfmodels.TaskTypeClusterUpgrade,
		kfmodels.OperationTypeClusterUpgrade, kfmodels.MetadataModifyCluster{}).Return(returnedOperation, nil)
}

func assertCreateFinishedTask(kafkaFixture kafkaFixture, resultOperation operations.Operation) {
	kafkaFixture.Tasks.EXPECT().
		CreateFinishedTask(
			kafkaFixture.Context,
			kafkaFixture.Session,
			clusterID,
			revisionID,
			kfmodels.OperationTypeClusterModify,
			nil,
			false).
		Return(resultOperation, nil)
}

func assertModifierModifyHost(kafkaFixture kafkaFixture, expectedArgs models.ModifyHostArgs, err error) {
	kafkaFixture.Modifier.EXPECT().
		ModifyHost(kafkaFixture.Context, expectedArgs).
		Return(err)
}

func assertModifyCluster(kafkaFixture kafkaFixture, changedSecurityGroups optional.Strings,
	expectedModifyClusterOptions internaltasks.ModifyClusterOptions, resultOperation operations.Operation) {
	kafkaFixture.Tasks.EXPECT().
		ModifyCluster(
			kafkaFixture.Context,
			kafkaFixture.Session,
			clusterID,
			revisionID,
			kfmodels.TaskTypeClusterModify,
			kfmodels.OperationTypeClusterModify,
			changedSecurityGroups,
			kafkaService,
			gomock.Any(),
			gomock.Any(),
			gomock.Any()).
		DoAndReturn(func(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasks.Type,
			opType operations.Type, securityGroupIDs optional.Strings, service string,
			attributesExtractor search.AttributesExtractor, opts ...internaltasks.ModifyClusterOption) (operations.Operation, error) {
			options := &internaltasks.ModifyClusterOptions{}
			for _, o := range opts {
				o(options)
			}
			require.Equal(kafkaFixture.T, &expectedModifyClusterOptions, options)

			return resultOperation, nil
		})
}

func assertSearchStoreDoc(kafkaFixture kafkaFixture, resultOperation operations.Operation) {
	kafkaFixture.Search.EXPECT().
		StoreDoc(
			kafkaFixture.Context,
			kafkaService,
			kafkaFixture.Session.FolderCoords.FolderExtID,
			kafkaFixture.Session.FolderCoords.CloudExtID,
			resultOperation,
			gomock.Any(),
		).
		Return(nil)
}

func assertModifierUpdatePillar(kafkaFixture kafkaFixture, expectedPillar kfpillars.Cluster) {
	kafkaFixture.Modifier.EXPECT().
		UpdatePillar(kafkaFixture.Context, clusterID, revisionID, gomock.Any()).
		DoAndReturn(func(ctx context.Context, cid string, revision int64, pillar *kfpillars.Cluster) error {
			require.Equal(kafkaFixture.T, expectedPillar, *pillar)
			return nil
		})
}

func minimalValidPillar() kfpillars.Cluster {
	return kfpillars.Cluster{
		Data: kfpillars.ClusterData{
			Kafka: kfpillars.KafkaData{
				Topics:                        map[string]kfpillars.TopicData{},
				DeletedTopics:                 map[string]kfpillars.DeletedTopic{},
				Users:                         map[string]kfpillars.UserData{},
				DeletedUsers:                  map[string]kfpillars.DeletedUser{},
				Nodes:                         map[string]kfpillars.KafkaNodeData{},
				Connectors:                    map[string]kfpillars.ConnectorData{},
				DeletedConnectors:             map[string]kfpillars.DeletedConnector{},
				MainBrokerID:                  1,
				UseCustomAuthorizerForCluster: true,
				AclsViaPy4j:                   true,
				UsePlainSasl:                  true,
				KnownTopicConfigProperties: []string{
					"cleanup.policy",
					"compression.type",
					"delete.retention.ms",
					"file.delete.delay.ms",
					"flush.messages",
					"flush.ms",
					"max.message.bytes",
					"min.compaction.lag.ms",
					"min.insync.replicas",
					"preallocate",
					"retention.bytes",
					"retention.ms",
					"segment.bytes",
				},
			},
			ZooKeeper: kfpillars.ZooKeeperData{
				Nodes: map[string]int{},
				Config: kfpillars.ZooKeeperConfig{
					DataDir: "/data/zookeper",
				},
			},
		},
	}
}

func copyPillar(t *testing.T, pillar *kfpillars.Cluster) *kfpillars.Cluster {
	jsonPillar, err := pillar.MarshalPillar()
	require.NoError(t, err)
	pillarCopy := kfpillars.NewCluster()
	err = pillarCopy.UnmarshalPillar(jsonPillar)
	require.NoError(t, err)
	return pillarCopy
}

func marshalPillar(t *testing.T, pillar *kfpillars.Cluster) json.RawMessage {
	pillarJSON, err := pillar.MarshalPillar()
	require.NoError(t, err)

	return pillarJSON
}
