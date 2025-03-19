package console

import (
	"context"

	chv1console "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/console/v1"
	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	consolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/clickhouse"
	consolecommon "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClusterService struct {
	chv1console.UnimplementedClusterServiceServer

	cfg     logic.CHConfig
	console console.Console
	ch      clickhouse.ClickHouse
}

var _ chv1console.ClusterServiceServer = &ClusterService{}

func NewClusterService(cfg logic.CHConfig, console console.Console, ch clickhouse.ClickHouse) *ClusterService {
	return &ClusterService{
		cfg:     cfg,
		console: console,
		ch:      ch,
	}
}

func (c *ClusterService) GetClustersConfig(ctx context.Context, req *chv1console.GetClustersConfigRequest) (*chv1console.ClustersConfig, error) {
	defaulVersion := ""
	versions := make([]*chv1.Version, 0, len(c.cfg.Versions))
	for _, ver := range c.cfg.Versions {
		grpcVer := grpcapi.VersionToGRPC(ver)

		versions = append(versions, grpcVer)

		if ver.Default {
			defaulVersion = grpcVer.Id
		}
	}

	cloudType, err := environment.ParseCloudType(req.CloudType)
	if err != nil {
		return nil, err
	}

	resourcePresets, err := c.console.GetResourcePresetsByCloudRegion(ctx, cloudType, req.GetRegionId(), clusters.TypeClickHouse, req.ProjectId)
	if err != nil {
		return nil, err
	}

	chResources := make([]*consolev1.ResourcePresetConfig, 0, len(resourcePresets)/2)
	for _, preset := range resourcePresets {
		switch preset.Role {
		case hosts.RoleClickHouse:
			chResources = append(chResources, datacloudgrpc.ResourcePresetToGRPC(preset))
		case hosts.RoleZooKeeper:
			continue
		default:
			return nil, xerrors.Errorf("invalid host role %q", preset.Role.String())
		}
	}

	chDefault, err := c.console.GetDefaultResourcesByClusterType(clusters.TypeClickHouse, hosts.RoleClickHouse)
	if err != nil {
		return nil, err
	}

	cls, err := c.console.ListClusters(ctx, req.GetProjectId(), optional.Int64{}, clusters.ClusterPageToken{})
	if err != nil {
		return nil, err
	}
	nameHint, err := consolecommon.GenerateClusterNameHint(clusters.TypeClickHouse, cls)
	if err != nil {
		return nil, err
	}

	return &chv1console.ClustersConfig{
		NameValidator: &consolev1.StringValidator{
			Regexp:    models.DefaultClusterNamePattern,
			Min:       models.DefaultClusterNameMinLen,
			Max:       models.DefaultClusterNameMaxLen,
			Blocklist: nil,
		},
		NameHint:       nameHint,
		Versions:       versions,
		DefaultVersion: defaulVersion,
		Clickhouse: &chv1console.ClustersConfig_Clickhouse{
			ResourcePresets:         chResources,
			DefaultResourcePresetId: chDefault.ResourcePresetExtID,
		},
	}, nil
}

func (c *ClusterService) EstimateCreate(ctx context.Context, req *chv1.CreateClusterRequest) (*consolev1.BillingEstimate, error) {
	createArgs, err := grpcapi.CreateClusterArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	billingEstimate, err := c.ch.EstimateCreateDCCluster(ctx, createArgs)
	if err != nil {
		return nil, err
	}

	return consolecommon.BillingEstimateToGRPC(billingEstimate), nil
}
