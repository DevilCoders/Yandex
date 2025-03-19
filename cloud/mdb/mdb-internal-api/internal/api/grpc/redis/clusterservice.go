package redis

import (
	"context"

	redisv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/redis/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	redisv1.UnimplementedClusterServiceServer

	cs    *grpcapi.ClusterService
	redis redis.Redis
}

var _ redisv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, redis redis.Redis) *ClusterService {
	return &ClusterService{cs: cs, redis: redis}
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *redisv1.ListClusterLogsRequest) (*redisv1.ListClusterLogsResponse, error) {
	lst, err := ListLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return nil, err
	}

	logsList, token, more, err := cs.cs.ListLogs(ctx, lst, req)
	if err != nil {
		return nil, err
	}

	var respLogs []*redisv1.LogRecord
	for _, l := range logsList {
		respLogs = append(respLogs, &redisv1.LogRecord{Timestamp: grpcapi.TimeToGRPC(l.Timestamp), Message: l.Message})
	}

	resp := &redisv1.ListClusterLogsResponse{Logs: respLogs}
	if more || req.GetAlwaysNextPageToken() {
		resp.NextPageToken = api.PagingTokenToGRPC(token)
	}

	return resp, nil
}

func (cs *ClusterService) StreamLogs(req *redisv1.StreamClusterLogsRequest, stream redisv1.ClusterService_StreamLogsServer) error {
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
				&redisv1.StreamLogRecord{
					Record: &redisv1.LogRecord{
						Timestamp: grpcapi.TimeToGRPC(l.Timestamp),
						Message:   l.Message,
					},
					NextRecordToken: api.PagingTokenToGRPC(l.NextMessageToken),
				},
			)
		},
	)
}

func (cs *ClusterService) StartFailover(ctx context.Context, req *redisv1.StartClusterFailoverRequest) (*operation.Operation, error) {
	op, err := cs.redis.StartFailover(
		ctx,
		req.GetClusterId(),
		req.GetHostNames(),
	)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Rebalance(ctx context.Context, req *redisv1.RebalanceClusterRequest) (*operation.Operation, error) {
	op, err := cs.redis.Rebalance(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) ListHosts(ctx context.Context, req *redisv1.ListClusterHostsRequest) (*redisv1.ListClusterHostsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &redisv1.ListClusterHostsResponse{}, err
	}

	hostsList, hostPageToken, err := cs.redis.ListHosts(ctx, req.ClusterId, req.GetPageSize(), pageToken.Offset)
	if err != nil {
		return &redisv1.ListClusterHostsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(hostPageToken, false)
	if err != nil {
		return &redisv1.ListClusterHostsResponse{}, err
	}

	return &redisv1.ListClusterHostsResponse{
		Hosts:         HostsToGRPC(hostsList),
		NextPageToken: nextPageToken,
	}, err
}

func (cs *ClusterService) AddHosts(ctx context.Context, req *redisv1.AddClusterHostsRequest) (*operation.Operation, error) {
	hostSpecs, err := HostSpecsFromGRPC(req.GetHostSpecs())
	if err != nil {
		return &operation.Operation{}, err
	}

	op, err := cs.redis.AddHosts(ctx, req.GetClusterId(), hostSpecs)
	if err != nil {
		return &operation.Operation{}, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Start(ctx context.Context, req *redisv1.StartClusterRequest) (*operation.Operation, error) {
	op, err := cs.redis.StartCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Stop(ctx context.Context, req *redisv1.StopClusterRequest) (*operation.Operation, error) {
	op, err := cs.redis.StopCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Move(ctx context.Context, req *redisv1.MoveClusterRequest) (*operation.Operation, error) {
	op, err := cs.redis.MoveCluster(ctx, req.GetClusterId(), req.GetDestinationFolderId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Backup(ctx context.Context, req *redisv1.BackupClusterRequest) (*operation.Operation, error) {
	op, err := cs.redis.BackupCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) ListBackups(ctx context.Context, req *redisv1.ListClusterBackupsRequest) (*redisv1.ListClusterBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), pageToken)
	if err != nil {
		return &redisv1.ListClusterBackupsResponse{}, err
	}

	backups, backupPageToken, err := cs.redis.ClusterBackups(ctx, req.ClusterId, pageToken, req.GetPageSize())
	if err != nil {
		return &redisv1.ListClusterBackupsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(backupPageToken, false)
	if err != nil {
		return &redisv1.ListClusterBackupsResponse{}, err
	}

	return &redisv1.ListClusterBackupsResponse{
		Backups:       BackupsToGRPC(backups),
		NextPageToken: nextPageToken,
	}, nil
}
