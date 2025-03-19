package console

import (
	"context"

	consolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	kfv1console "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/console/v1"
	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc"
	consolecommon "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/console"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClusterService struct {
	kfv1console.UnimplementedClusterServiceServer

	cfg     logic.KafkaConfig
	console console.Console
	kf      kafka.Kafka
}

var _ kfv1console.ClusterServiceServer = &ClusterService{}

func NewClusterService(cfg logic.KafkaConfig, console console.Console, kf kafka.Kafka) *ClusterService {
	return &ClusterService{
		cfg:     cfg,
		console: console,
		kf:      kf,
	}
}

func (c *ClusterService) GetClustersConfig(ctx context.Context, req *kfv1console.GetClustersConfigRequest) (*kfv1console.ClustersConfig, error) {
	cloudType, err := environment.ParseCloudType(req.CloudType)
	if err != nil {
		return nil, err
	}

	resourcePresets, err := c.console.GetResourcePresetsByCloudRegion(ctx, cloudType, req.GetRegionId(), clusters.TypeKafka, req.ProjectId)
	if err != nil {
		return nil, err
	}

	var kfResources []*consolev1.ResourcePresetConfig
	for _, preset := range resourcePresets {
		switch preset.Role {
		case hosts.RoleKafka:
			kfResources = append(kfResources, datacloudgrpc.ResourcePresetToGRPC(preset))
		case hosts.RoleZooKeeper:
			// don't show zk resources in console API
		default:
			return nil, xerrors.Errorf("invalid host role %q", preset.Role.String())
		}
	}

	kfDefault, err := c.console.GetDefaultResourcesByClusterType(clusters.TypeKafka, hosts.RoleKafka)
	if err != nil {
		return nil, err
	}

	cls, err := c.console.ListClusters(ctx, req.GetProjectId(), optional.Int64{}, clusters.ClusterPageToken{})
	if err != nil {
		return nil, err
	}
	nameHint, err := consolecommon.GenerateClusterNameHint(clusters.TypeKafka, cls)
	if err != nil {
		return nil, err
	}

	return &kfv1console.ClustersConfig{
		NameValidator: &consolev1.StringValidator{
			Regexp:    models.DefaultClusterNamePattern,
			Min:       models.DefaultClusterNameMinLen,
			Max:       models.DefaultClusterNameMaxLen,
			Blocklist: nil,
		},
		NameHint:       nameHint,
		Versions:       grpcapi.VersionsToGRPC(),
		DefaultVersion: kfmodels.DefaultVersion,
		Kafka: &kfv1console.ClustersConfig_Kafka{
			ResourcePresets:         kfResources,
			DefaultResourcePresetId: kfDefault.ResourcePresetExtID,
		},
	}, nil
}

func (c *ClusterService) EstimateCreate(ctx context.Context, req *kfv1.CreateClusterRequest) (*consolev1.BillingEstimate, error) {
	createArgs, err := grpcapi.CreateClusterArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	billingEstimate, err := c.kf.EstimateCreateDCCluster(ctx, createArgs)
	if err != nil {
		return nil, err
	}

	return consolecommon.BillingEstimateToGRPC(billingEstimate), nil
}
