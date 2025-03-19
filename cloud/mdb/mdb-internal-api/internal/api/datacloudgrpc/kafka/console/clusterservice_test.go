package console

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	consolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	kfv1console "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/console/v1"
	v11 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	consolemocks "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

type fixture struct {
	ClusterService *ClusterService
	Context        context.Context
	Console        *consolemocks.MockConsole
}

func newFixture(t *testing.T) *fixture {
	ctrl := gomock.NewController(t)
	console := consolemocks.NewMockConsole(ctrl)
	kf := mocks.NewMockKafka(ctrl)
	return &fixture{
		ClusterService: NewClusterService(logic.KafkaConfig{}, console, kf),
		Context:        context.Background(),
		Console:        console,
	}
}

func Test_GetClustersConfig(t *testing.T) {
	availableVersions := []*v11.Version{
		{
			Id:          "3.1",
			Name:        "3.1",
			Deprecated:  false,
			UpdatableTo: []string{},
		},
		{
			Id:          "3.0",
			Name:        "3.0",
			Deprecated:  false,
			UpdatableTo: []string{"3.1"},
		},
		{
			Id:          "2.8",
			Name:        "2.8",
			Deprecated:  false,
			UpdatableTo: []string{"3.0", "3.1"},
		},
	}
	nameValidator := &consolev1.StringValidator{
		Regexp:    models.DefaultClusterNamePattern,
		Min:       models.DefaultClusterNameMinLen,
		Max:       models.DefaultClusterNameMaxLen,
		Blocklist: nil,
	}

	fixture := newFixture(t)

	t.Run("When e[ty cloud type should return error", func(t *testing.T) {
		req := kfv1console.GetClustersConfigRequest{}
		_, err := fixture.ClusterService.GetClustersConfig(fixture.Context, &req)
		require.EqualError(t, err, "invalid cloud type: \"\"")
	})

	t.Run("When wrong cloud type should return error", func(t *testing.T) {
		req := kfv1console.GetClustersConfigRequest{
			CloudType: "wrongCloudType",
		}
		_, err := fixture.ClusterService.GetClustersConfig(fixture.Context, &req)
		require.EqualError(t, err, "invalid cloud type: \"wrongCloudType\"")
	})

	t.Run("When GetResourcePresetsByCloudRegion returns error should return error", func(t *testing.T) {
		req := kfv1console.GetClustersConfigRequest{
			CloudType: string(environment.CloudTypeAWS),
			RegionId:  "regionId",
			ProjectId: "projectId",
		}
		fixture.Console.EXPECT().
			GetResourcePresetsByCloudRegion(fixture.Context, environment.CloudTypeAWS, "regionId", clusters.TypeKafka, "projectId").
			Return(nil, semerr.InvalidInput("not found resource preset"))

		_, err := fixture.ClusterService.GetClustersConfig(fixture.Context, &req)

		require.EqualError(t, err, "not found resource preset")
	})

	t.Run("When GetDefaultResourcesByClusterType returns error should return error", func(t *testing.T) {
		req := kfv1console.GetClustersConfigRequest{
			CloudType: string(environment.CloudTypeAWS),
			RegionId:  "regionId",
			ProjectId: "projectId",
		}
		fixture.Console.EXPECT().
			GetResourcePresetsByCloudRegion(fixture.Context, environment.CloudTypeAWS, "regionId", clusters.TypeKafka, "projectId").
			Return([]consolemodels.ResourcePreset{}, nil)
		fixture.Console.EXPECT().
			GetDefaultResourcesByClusterType(clusters.TypeKafka, hosts.RoleKafka).
			Return(consolemodels.DefaultResources{}, semerr.InvalidInput("not found default resources for kafka"))

		_, err := fixture.ClusterService.GetClustersConfig(fixture.Context, &req)

		require.EqualError(t, err, "not found default resources for kafka")
	})

	t.Run("When GetDefaultResourcesByClusterType returns error should return error", func(t *testing.T) {
		req := kfv1console.GetClustersConfigRequest{
			CloudType: string(environment.CloudTypeAWS),
			RegionId:  "regionId",
			ProjectId: "projectId",
		}
		fixture.Console.EXPECT().
			GetResourcePresetsByCloudRegion(fixture.Context, environment.CloudTypeAWS, "regionId", clusters.TypeKafka, "projectId").
			Return([]consolemodels.ResourcePreset{}, nil)
		fixture.Console.EXPECT().
			GetDefaultResourcesByClusterType(clusters.TypeKafka, hosts.RoleKafka).
			Return(consolemodels.DefaultResources{}, semerr.InvalidInput("not found default resources for kafka"))

		_, err := fixture.ClusterService.GetClustersConfig(fixture.Context, &req)

		require.EqualError(t, err, "not found default resources for kafka")
	})

	t.Run("When GetDefaultResourcesByClusterType returns error should return error", func(t *testing.T) {
		req := kfv1console.GetClustersConfigRequest{
			CloudType: string(environment.CloudTypeAWS),
			RegionId:  "regionId",
			ProjectId: "projectId",
		}
		fixture.Console.EXPECT().
			GetResourcePresetsByCloudRegion(fixture.Context, environment.CloudTypeAWS, "regionId", clusters.TypeKafka, "projectId").
			Return([]consolemodels.ResourcePreset{}, nil)
		fixture.Console.EXPECT().
			GetDefaultResourcesByClusterType(clusters.TypeKafka, hosts.RoleKafka).
			Return(consolemodels.DefaultResources{}, nil)
		fixture.Console.EXPECT().
			ListClusters(fixture.Context, "projectId", optional.Int64{}, clusters.ClusterPageToken{}).
			Return([]consolemodels.Cluster{}, semerr.InvalidInput("can't get clusters"))

		_, err := fixture.ClusterService.GetClustersConfig(fixture.Context, &req)

		require.EqualError(t, err, "can't get clusters")
	})

	t.Run("When no errors and get default parameters should return answer with default parameters", func(t *testing.T) {
		req := kfv1console.GetClustersConfigRequest{
			CloudType: string(environment.CloudTypeAWS),
			RegionId:  "regionId",
			ProjectId: "projectId",
		}
		fixture.Console.EXPECT().
			GetResourcePresetsByCloudRegion(fixture.Context, environment.CloudTypeAWS, "regionId", clusters.TypeKafka, "projectId").
			Return([]consolemodels.ResourcePreset{}, nil)
		fixture.Console.EXPECT().
			GetDefaultResourcesByClusterType(clusters.TypeKafka, hosts.RoleKafka).
			Return(consolemodels.DefaultResources{}, nil)
		fixture.Console.EXPECT().
			ListClusters(fixture.Context, "projectId", optional.Int64{}, clusters.ClusterPageToken{}).
			Return([]consolemodels.Cluster{}, nil)

		result, err := fixture.ClusterService.GetClustersConfig(fixture.Context, &req)

		require.NoError(t, err)
		require.Equal(t, &kfv1console.ClustersConfig{
			NameValidator:  nameValidator,
			NameHint:       "kafka1",
			Versions:       availableVersions,
			DefaultVersion: kfmodels.DefaultVersion,
			Kafka: &kfv1console.ClustersConfig_Kafka{
				ResourcePresets:         nil,
				DefaultResourcePresetId: "",
			},
		}, result)
	})

	t.Run("When have resources are kafka and zk should return only kafka resources", func(t *testing.T) {
		req := kfv1console.GetClustersConfigRequest{
			CloudType: string(environment.CloudTypeAWS),
			RegionId:  "regionId",
			ProjectId: "projectId",
		}
		fixture.Console.EXPECT().
			GetResourcePresetsByCloudRegion(fixture.Context, environment.CloudTypeAWS, "regionId", clusters.TypeKafka, "projectId").
			Return([]consolemodels.ResourcePreset{
				{
					Role:            hosts.RoleKafka,
					ExtID:           "kafkaResourcePresetId1",
					CPULimit:        100,
					MemoryLimit:     200,
					DiskSizes:       []int64{300, 400, 500},
					DefaultDiskSize: optional.NewInt64(400),
				},
				{
					Role:        hosts.RoleKafka,
					ExtID:       "kafkaResourcePresetId2",
					CPULimit:    2100,
					MemoryLimit: 2200,
					DiskSizes:   []int64{2300, 2400, 2500},
				},
				{
					Role:            hosts.RoleZooKeeper,
					ExtID:           "zkResourcePresetId1",
					CPULimit:        1000,
					MemoryLimit:     2000,
					DiskSizes:       []int64{3000, 4000, 5000},
					DefaultDiskSize: optional.NewInt64(4000),
				},
				{
					Role:        hosts.RoleZooKeeper,
					ExtID:       "zkResourcePresetId2",
					CPULimit:    31000,
					MemoryLimit: 32000,
					DiskSizes:   []int64{33000, 34000, 35000},
				},
			}, nil)
		fixture.Console.EXPECT().
			GetDefaultResourcesByClusterType(clusters.TypeKafka, hosts.RoleKafka).
			Return(consolemodels.DefaultResources{
				ResourcePresetExtID: "defaultResourcePresetId",
			}, nil)
		fixture.Console.EXPECT().
			ListClusters(fixture.Context, "projectId", optional.Int64{}, clusters.ClusterPageToken{}).
			Return([]consolemodels.Cluster{
				{
					Type: clusters.TypeKafka,
					Name: "kafka1",
				},
				{
					Type: clusters.TypeKafka,
					Name: "kafka2",
				},
			}, nil)

		result, err := fixture.ClusterService.GetClustersConfig(fixture.Context, &req)

		require.NoError(t, err)
		require.Equal(t, &kfv1console.ClustersConfig{
			NameValidator:  nameValidator,
			NameHint:       "kafka3",
			Versions:       availableVersions,
			DefaultVersion: kfmodels.DefaultVersion,
			Kafka: &kfv1console.ClustersConfig_Kafka{
				ResourcePresets: []*consolev1.ResourcePresetConfig{
					{
						ResourcePresetId: "kafkaResourcePresetId1",
						CpuLimit:         100,
						MemoryLimit:      200,
						MinDiskSize:      300,
						MaxDiskSize:      500,
						DiskSizes:        []int64{300, 400, 500},
						DefaultDiskSize:  400,
					},
					{
						ResourcePresetId: "kafkaResourcePresetId2",
						CpuLimit:         2100,
						MemoryLimit:      2200,
						MinDiskSize:      2300,
						MaxDiskSize:      2500,
						DiskSizes:        []int64{2300, 2400, 2500},
						DefaultDiskSize:  2300,
					},
				},
				DefaultResourcePresetId: "defaultResourcePresetId",
			},
		}, result)
	})
}
