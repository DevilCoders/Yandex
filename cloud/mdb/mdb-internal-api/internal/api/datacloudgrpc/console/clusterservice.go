package console

import (
	"context"

	consolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

type ClusterService struct {
	consolev1.UnimplementedClusterServiceServer

	console console.Console
}

var _ consolev1.ClusterServiceServer = &ClusterService{}

func NewClusterService(console console.Console) *ClusterService {
	return &ClusterService{
		console: console,
	}
}

func (c *ClusterService) List(ctx context.Context, req *consolev1.ListClustersRequest) (*consolev1.ListClustersResponse, error) {
	var pageToken clustermodels.ClusterPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	pageSize := pagination.SanePageSize(req.GetPaging().GetPageSize())

	clusters, err := c.console.ListClusters(ctx,
		req.GetProjectId(),
		optional.NewInt64(pageSize),
		pageToken,
	)
	if err != nil {
		return nil, err
	}

	grpcClusters := make([]*consolev1.Cluster, 0, len(clusters))
	for _, cluster := range clusters {
		grpcClusters = append(grpcClusters, &consolev1.Cluster{
			Id:          cluster.ID,
			ProjectId:   cluster.ProjectID,
			CloudType:   string(cluster.CloudType),
			RegionId:    cluster.RegionID,
			CreateTime:  grpc.TimeToGRPC(cluster.CreateTime),
			Name:        cluster.Name,
			Description: cluster.Description,
			Status:      datacloudgrpc.StatusToGRPC(cluster.Status, cluster.Health),
			Type:        datacloudgrpc.ClusterTypeToGRPC(cluster.Type),
			Version:     cluster.Version,
			Resources:   datacloudgrpc.AllClusterResourcesToGRPC(cluster.Type, cluster.Resources),
		})
	}

	clusterPageToken := consolemodels.NewConsoleClusterPageToken(clusters, pageSize)
	nextPageToken, err := api.BuildPageTokenToGRPC(clusterPageToken, false)
	if err != nil {
		return nil, err
	}

	return &consolev1.ListClustersResponse{
		Clusters: grpcClusters,
		NextPage: &apiv1.NextPage{Token: nextPageToken},
	}, nil
}

func (c *ClusterService) ProjectByClusterID(ctx context.Context, req *consolev1.ProjectByClusterIDRequest) (*consolev1.ProjectByClusterIDResponse, error) {
	projectID, err := c.console.ProjectByClusterID(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &consolev1.ProjectByClusterIDResponse{ProjectId: projectID}, nil
}
