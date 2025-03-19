package metastore

import (
	"context"

	msv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/metastore/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/models"
	"a.yandex-team.ru/library/go/slices"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	msv1.UnimplementedClusterServiceServer

	cs  *grpcapi.ClusterService
	ms  metastore.Metastore
	ops common.Operations
}

var _ msv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, ms metastore.Metastore, ops common.Operations) *ClusterService {
	return &ClusterService{
		cs:  cs,
		ms:  ms,
		ops: ops,
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *msv1.GetClusterRequest) (*msv1.Cluster, error) {
	cluster, err := cs.ms.MDBCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}
	return ClusterToGRPC(cluster), nil
}

func (cs *ClusterService) List(ctx context.Context, req *msv1.ListClustersRequest) (*msv1.ListClustersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	clusters, err := cs.ms.MDBClusters(ctx, req.GetFolderId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &msv1.ListClustersResponse{Clusters: ClustersToGRPC(clusters)}, nil
}

func ClustersToGRPC(clusters []models.MDBCluster) []*msv1.Cluster {
	var v []*msv1.Cluster
	for _, cluster := range clusters {
		v = append(v, ClusterToGRPC(cluster))
	}
	return v
}

func (cs *ClusterService) Create(ctx context.Context, req *msv1.CreateClusterRequest) (*operation.Operation, error) {
	createClusterArgs := metastore.CreateMDBClusterArgs{
		FolderExtID:        req.GetFolderId(),
		Name:               req.GetName(),
		Description:        req.GetDescription(),
		Labels:             req.GetLabels(),
		SecurityGroupIDs:   slices.DedupStrings(req.GetSecurityGroupIds()),
		HostGroupIDs:       slices.DedupStrings(req.GetHostGroupIds()),
		DeletionProtection: req.GetDeletionProtection(),
		Version:            req.GetVersion(),
		MinServersPerZone:  req.GetMinServersPerZone(),
		MaxServersPerZone:  req.GetMaxServersPerZone(),
		SubnetIDs:          req.GetSubnetIds(),
	}
	op, err := cs.ms.CreateMDBCluster(ctx, createClusterArgs)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Delete(ctx context.Context, req *msv1.DeleteClusterRequest) (*operation.Operation, error) {
	op, err := cs.ms.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) ListOperations(ctx context.Context, req *msv1.ListClusterOperationsRequest) (*msv1.ListClusterOperationsResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}
	ops, err := cs.ops.OperationsByClusterID(ctx, req.GetClusterId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	opsResponse := make([]*operation.Operation, 0, len(ops))
	for _, op := range ops {
		opResponse, err := grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
		if err != nil {
			return nil, err
		}
		opsResponse = append(opsResponse, opResponse)
	}

	return &msv1.ListClusterOperationsResponse{Operations: opsResponse}, nil
}
