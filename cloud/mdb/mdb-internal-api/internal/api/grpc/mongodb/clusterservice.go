package mongodb

import (
	"context"

	mongov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	mongov1.UnimplementedClusterServiceServer

	cs            *grpcapi.ClusterService
	mongo         mongodb.MongoDB
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ mongov1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, mongo mongodb.MongoDB, saltEnvsCfg logic.SaltEnvsConfig) *ClusterService {
	return &ClusterService{
		cs:    cs,
		mongo: mongo,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(mongov1.Cluster_PRODUCTION),
			int64(mongov1.Cluster_PRESTABLE),
			int64(mongov1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *mongov1.GetClusterRequest) (*mongov1.Cluster, error) {
	cluster, err := cs.mongo.Cluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return ClusterToGRPC(cluster), nil
}

func (cs *ClusterService) List(ctx context.Context, req *mongov1.ListClustersRequest) (*mongov1.ListClustersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	clusters, err := cs.mongo.Clusters(ctx, req.GetFolderId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &mongov1.ListClustersResponse{Clusters: ClustersToGRPC(clusters)}, nil
}

func (cs *ClusterService) Create(ctx context.Context, req *mongov1.CreateClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}

	op, err := cs.mongo.CreateCluster(
		ctx,
		mongodb.CreateClusterArgs{
			FolderExtID:        req.GetFolderId(),
			Name:               req.GetName(),
			Description:        req.GetDescription(),
			Labels:             req.GetLabels(),
			Environment:        env,
			UserSpecs:          UserSpecsFromGRPC(req.GetUserSpecs()),
			DatabaseSpecs:      DatabaseSpecsFromGRPC(req.GetDatabaseSpecs()),
			NetworkID:          req.GetNetworkId(),
			SecurityGroupIDs:   req.GetSecurityGroupIds(),
			DeletionProtection: req.GetDeletionProtection(),
			// TODO: add ClusterConfigSpec and HostSpecs
		})
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Delete(ctx context.Context, req *mongov1.DeleteClusterRequest) (*operation.Operation, error) {
	op, err := cs.mongo.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *mongov1.ListClusterLogsRequest) (*mongov1.ListClusterLogsResponse, error) {
	lst, err := ListLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return nil, err
	}

	logsList, token, more, err := cs.cs.ListLogs(ctx, lst, req)
	if err != nil {
		return nil, err
	}

	var respLogs []*mongov1.LogRecord
	for _, l := range logsList {
		respLogs = append(respLogs, &mongov1.LogRecord{Timestamp: grpcapi.TimeToGRPC(l.Timestamp), Message: l.Message})
	}

	resp := &mongov1.ListClusterLogsResponse{Logs: respLogs}
	if more || req.GetAlwaysNextPageToken() {
		resp.NextPageToken = api.PagingTokenToGRPC(token)
	}

	return resp, nil
}

func (cs *ClusterService) StreamLogs(req *mongov1.StreamClusterLogsRequest, stream mongov1.ClusterService_StreamLogsServer) error {
	lst, err := StreamLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return err
	}

	return cs.cs.StreamLogs(
		stream.Context(),
		lst,
		req,
		func(l logs.Message) error {
			return stream.Send(
				&mongov1.StreamLogRecord{
					Record: &mongov1.LogRecord{
						Timestamp: grpcapi.TimeToGRPC(l.Timestamp),
						Message:   l.Message,
					},
					NextRecordToken: api.PagingTokenToGRPC(l.NextMessageToken),
				},
			)
		},
	)
}

func (cs *ClusterService) ResetupHosts(ctx context.Context, req *mongov1.ResetupHostsRequest) (*operation.Operation, error) {
	op, err := cs.mongo.ResetupHosts(
		ctx,
		req.GetClusterId(),
		req.GetHostNames(),
	)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) StepdownHosts(ctx context.Context, req *mongov1.StepdownHostsRequest) (*operation.Operation, error) {
	op, err := cs.mongo.StepdownHosts(
		ctx,
		req.GetClusterId(),
		req.GetHostNames(),
	)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) RestartHosts(ctx context.Context, req *mongov1.RestartHostsRequest) (*operation.Operation, error) {
	op, err := cs.mongo.RestartHosts(
		ctx,
		req.GetClusterId(),
		req.GetHostNames(),
	)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}
