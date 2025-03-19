package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	chv1.UnimplementedClusterServiceServer

	cs *grpcapi.ClusterService
	ch clickhouse.ClickHouse
}

var _ chv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, ch clickhouse.ClickHouse) *ClusterService {
	return &ClusterService{
		cs: cs,
		ch: ch,
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *chv1.GetClusterRequest) (*chv1.Cluster, error) {
	cluster, err := cs.ch.DataCloudCluster(ctx, req.GetClusterId(), req.GetSensitive())
	if err != nil {
		return nil, err
	}
	return ClusterToGRPC(cluster, cs.cs.Config.Domain), nil
}

func (cs *ClusterService) List(ctx context.Context, req *chv1.ListClustersRequest) (*chv1.ListClustersResponse, error) {
	var pageToken clustermodels.ClusterPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	pageSize := pagination.SanePageSize(req.GetPaging().GetPageSize())

	clusters, err := cs.ch.DataCloudClusters(ctx, req.GetProjectId(), pageSize, pageToken)
	if err != nil {
		return nil, err
	}

	clusterPageToken := chmodels.NewDataCloudClusterPageToken(clusters, pageSize)

	nextPageToken, err := api.BuildPageTokenToGRPC(clusterPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListClustersResponse{
		Clusters: ClustersToGRPC(clusters, cs.cs.Config.Domain),
		NextPage: &apiv1.NextPage{
			Token: nextPageToken,
		},
	}, nil
}

func (cs *ClusterService) Create(ctx context.Context, req *chv1.CreateClusterRequest) (*chv1.CreateClusterResponse, error) {
	args, err := CreateClusterArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}
	op, err := cs.ch.CreateDataCloudCluster(ctx, args)
	if err != nil {
		return nil, err
	}
	return &chv1.CreateClusterResponse{
		ClusterId:   op.ClusterID,
		OperationId: op.OperationID,
	}, nil
}

func (cs *ClusterService) Update(ctx context.Context, req *chv1.UpdateClusterRequest) (*chv1.UpdateClusterResponse, error) {
	op, err := cs.ch.ModifyDataCloudCluster(ctx, UpdateClusterArgsFromGRPC(req))
	if err != nil {
		return nil, err
	}

	return &chv1.UpdateClusterResponse{OperationId: op.OperationID}, nil
}

func (cs *ClusterService) Delete(ctx context.Context, req *chv1.DeleteClusterRequest) (*chv1.DeleteClusterResponse, error) {
	op, err := cs.ch.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &chv1.DeleteClusterResponse{OperationId: op.OperationID}, nil
}

func (cs *ClusterService) ListHosts(ctx context.Context, req *chv1.ListClusterHostsRequest) (*chv1.ListClusterHostsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return &chv1.ListClusterHostsResponse{}, err
	}

	hosts, hostPageToken, err := cs.ch.ListHosts(ctx, req.GetClusterId(), req.GetPaging().GetPageSize(), pageToken.Offset)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(hostPageToken, false)
	if err != nil {
		return &chv1.ListClusterHostsResponse{}, err
	}

	grpcHosts := HostsToGRPC(hosts)
	for _, h := range grpcHosts {
		h.PrivateName = switchToPrivateDomain(h.Name, cs.cs.Config.Domain)
	}
	return &chv1.ListClusterHostsResponse{
		Hosts:    grpcHosts,
		NextPage: &apiv1.NextPage{Token: nextPageToken},
	}, nil
}

func (cs *ClusterService) ResetCredentials(ctx context.Context, req *chv1.ResetClusterCredentialsRequest) (*chv1.ResetClusterCredentialsResponse, error) {
	op, err := cs.ch.ResetCredentials(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &chv1.ResetClusterCredentialsResponse{OperationId: op.OperationID}, err
}

func (cs *ClusterService) Backup(ctx context.Context, req *chv1.BackupClusterRequest) (*chv1.BackupClusterResponse, error) {
	op, err := cs.ch.BackupCluster(ctx, req.GetClusterId(), optional.NewString(req.GetName()))
	if err != nil {
		return nil, err
	}

	return &chv1.BackupClusterResponse{OperationId: op.OperationID}, nil
}

func (cs *ClusterService) Restore(ctx context.Context, req *chv1.RestoreClusterRequest) (*chv1.RestoreClusterResponse, error) {
	args, err := restoreClusterArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := cs.ch.RestoreDataCloudCluster(ctx, req.GetBackupId(), args)
	if err != nil {
		return nil, err
	}
	return &chv1.RestoreClusterResponse{
		ClusterId:   op.ClusterID,
		OperationId: op.OperationID,
	}, nil
}

func (cs *ClusterService) ListBackups(ctx context.Context, req *chv1.ListClusterBackupsRequest) (*chv1.ListClusterBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return &chv1.ListClusterBackupsResponse{}, err
	}

	backups, backupPageToken, err := cs.ch.ClusterBackups(ctx, req.ClusterId, pageToken, req.GetPaging().GetPageSize())
	if err != nil {
		return nil, err
	}

	grpcToken, err := api.BuildPageTokenToGRPC(backupPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListClusterBackupsResponse{
		Backups: BackupsToGRPC(backups),
		NextPage: &apiv1.NextPage{
			Token: grpcToken,
		},
	}, nil
}

func (cs *ClusterService) Start(ctx context.Context, req *chv1.StartClusterRequest) (*chv1.StartClusterResponse, error) {
	op, err := cs.ch.StartCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &chv1.StartClusterResponse{OperationId: op.OperationID}, nil
}

func (cs *ClusterService) Stop(ctx context.Context, req *chv1.StopClusterRequest) (*chv1.StopClusterResponse, error) {
	op, err := cs.ch.StopCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &chv1.StopClusterResponse{OperationId: op.OperationID}, nil
}
