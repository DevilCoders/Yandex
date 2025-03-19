package console

import (
	"context"

	consolev1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/console"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ClusterService implements gRPC methods for CUD console helpers
type ClusterService struct {
	consolev1.UnimplementedClusterServiceServer

	Console console.Console
	L       log.Logger
}

var _ consolev1.ClusterServiceServer = &ClusterService{}

// GetUsedResources returns used resources in clouds by cluster types
func (cs *ClusterService) GetUsedResources(ctx context.Context, req *consolev1.GetUsedResourcesRequest) (*consolev1.GetUsedResourcesResponse, error) {
	resources, err := cs.Console.GetUsedResources(ctx, req.CloudIds)
	if err != nil {
		return nil, err
	}

	respUsedResources := make([]*consolev1.GetUsedResourcesResponse_UsedResourcesInCloud, 0, len(resources))
	for _, res := range resources {
		cType, err := userResourcesClusterTypeFromModelToGRPC(res.ClusterType)
		if err != nil {
			return nil, err
		}

		respUsedResources = append(
			respUsedResources,
			&consolev1.GetUsedResourcesResponse_UsedResourcesInCloud{
				CloudId:     res.CloudID,
				ClusterType: cType,
				Role:        res.Role.Stringified(),
				PlatformId:  string(res.PlatformID),
				Cores:       res.Cores,
				Memory:      res.Memory,
			})
	}

	return &consolev1.GetUsedResourcesResponse{Resources: respUsedResources}, nil
}

func userResourcesClusterTypeFromModelToGRPC(modelType clusters.Type) (consolev1.GetUsedResourcesResponse_Type, error) {
	switch modelType {
	case clusters.TypeClickHouse:
		return consolev1.GetUsedResourcesResponse_CLICKHOUSE, nil
	case clusters.TypePostgreSQL:
		return consolev1.GetUsedResourcesResponse_POSTGRESQL, nil
	case clusters.TypeMongoDB:
		return consolev1.GetUsedResourcesResponse_MONGODB, nil
	case clusters.TypeRedis:
		return consolev1.GetUsedResourcesResponse_REDIS, nil
	case clusters.TypeMySQL:
		return consolev1.GetUsedResourcesResponse_MYSQL, nil
	case clusters.TypeKafka:
		return consolev1.GetUsedResourcesResponse_KAFKA, nil
	case clusters.TypeSQLServer:
		return consolev1.GetUsedResourcesResponse_SQLSERVER, nil
	case clusters.TypeElasticSearch:
		return consolev1.GetUsedResourcesResponse_ELASTICSEARCH, nil
	case clusters.TypeGreenplumCluster:
		return consolev1.GetUsedResourcesResponse_GREENPLUM, nil
	default:
		return consolev1.GetUsedResourcesResponse_TYPE_UNSPECIFIED, xerrors.Errorf("unknown cluster type: %s", modelType)
	}
}

func (cs *ClusterService) GetFolderStats(ctx context.Context, req *consolev1.GetFolderStatsRequest) (*consolev1.FolderStats, error) {
	stats, err := cs.Console.FolderStats(ctx, req.GetFolderId())
	if err != nil {
		return nil, err
	}

	var res consolev1.FolderStats
	for k, v := range stats.Clusters {
		typ, err := clusterTypeFromModelToGRPC(k)
		if err != nil {
			cs.L.Error("cannot convert cluster type to gRPC model", log.Error(err))
			sentry.GlobalClient().CaptureError(ctx, err, nil)
			continue
		}

		res.ClusterStats = append(res.ClusterStats, &consolev1.FolderStats_ClusterStats{ClusterType: typ, ClustersCount: v})
	}

	return &res, nil
}

func (cs *ClusterService) GetDiskIOLimit(ctx context.Context, req *consolev1.GetDiskIOLimitRequest) (*consolev1.GetDiskIOLimitResponse, error) {
	ioLimit, err := cs.Console.GetDiskIOLimit(ctx, req.GetSpaceLimit(), req.GetDiskType(), req.GetResourcePreset())
	if err != nil {
		return nil, err
	}
	return &consolev1.GetDiskIOLimitResponse{IoLimit: ioLimit}, nil
}
