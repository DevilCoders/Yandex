package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
)

type ClusterService struct {
	kfv1.UnimplementedClusterServiceServer

	cs  *grpcapi.ClusterService
	kf  kafka.Kafka
	ops common.Operations
}

func NewClusterService(cs *grpcapi.ClusterService, kf kafka.Kafka, ops common.Operations) *ClusterService {
	return &ClusterService{
		cs:  cs,
		kf:  kf,
		ops: ops,
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *kfv1.GetClusterRequest) (*kfv1.Cluster, error) {
	cluster, err := cs.kf.DataCloudCluster(ctx, req.GetClusterId(), req.GetSensitive())
	if err != nil {
		return nil, err
	}

	return ClusterToGRPC(cluster), nil
}

func (cs *ClusterService) List(ctx context.Context, req *kfv1.ListClustersRequest) (*kfv1.ListClustersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPaging().GetPageToken())
	if err != nil {
		return nil, err
	}

	clusters, err := cs.kf.DataCloudClusters(ctx, req.GetProjectId(), req.GetPaging().GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &kfv1.ListClustersResponse{Clusters: ClustersToGRPC(clusters)}, nil
}

func (cs *ClusterService) ListHosts(ctx context.Context, req *kfv1.ListClusterHostsRequest) (*kfv1.ListClusterHostsResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPaging().GetPageToken())
	if err != nil {
		return nil, err
	}
	hosts, err := cs.kf.ListHosts(ctx, req.GetClusterId(), req.GetPaging().GetPageSize(), offset)
	if err != nil {
		return nil, err
	}
	hs, err := HostsToGRPC(hosts)
	if err != nil {
		return nil, err
	}
	return &kfv1.ListClusterHostsResponse{
		Hosts: hs,
	}, err
}

func (cs *ClusterService) Create(ctx context.Context, req *kfv1.CreateClusterRequest) (*kfv1.CreateClusterResponse, error) {
	args, err := CreateClusterArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := cs.kf.CreateDataCloudCluster(ctx, args)
	if err != nil {
		return nil, err
	}

	return &kfv1.CreateClusterResponse{
		OperationId: op.OperationID,
		ClusterId:   op.ClusterID,
	}, nil
}

func (cs *ClusterService) Update(ctx context.Context, req *kfv1.UpdateClusterRequest) (*kfv1.UpdateClusterResponse, error) {
	args, err := modifyClusterArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}
	op, err := cs.kf.ModifyDataCloudCluster(ctx, args)
	if err != nil {
		return nil, err
	}

	return &kfv1.UpdateClusterResponse{
		OperationId: op.OperationID,
	}, nil
}

func (cs *ClusterService) Delete(ctx context.Context, req *kfv1.DeleteClusterRequest) (*kfv1.DeleteClusterResponse, error) {
	op, err := cs.kf.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &kfv1.DeleteClusterResponse{OperationId: op.OperationID}, nil
}

func (cs *ClusterService) ResetCredentials(ctx context.Context, req *kfv1.ResetClusterCredentialsRequest) (*kfv1.ResetClusterCredentialsResponse, error) {
	op, err := cs.kf.ResetCredentials(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &kfv1.ResetClusterCredentialsResponse{OperationId: op.OperationID}, err
}

func (cs *ClusterService) Start(ctx context.Context, req *kfv1.StartClusterRequest) (*kfv1.StartClusterResponse, error) {
	op, err := cs.kf.StartCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &kfv1.StartClusterResponse{OperationId: op.OperationID}, nil
}

func (cs *ClusterService) Stop(ctx context.Context, req *kfv1.StopClusterRequest) (*kfv1.StopClusterResponse, error) {
	op, err := cs.kf.StopCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return &kfv1.StopClusterResponse{OperationId: op.OperationID}, nil
}
