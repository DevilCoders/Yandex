package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	chv1.UnimplementedClusterServiceServer

	cs            *grpcapi.ClusterService
	ch            clickhouse.ClickHouse
	ops           common.Operations
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ chv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, ch clickhouse.ClickHouse, ops common.Operations, saltEnvsCfg logic.SaltEnvsConfig) *ClusterService {
	return &ClusterService{
		cs:  cs,
		ch:  ch,
		ops: ops,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(chv1.Cluster_PRODUCTION),
			int64(chv1.Cluster_PRESTABLE),
			int64(chv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *chv1.GetClusterRequest) (*chv1.Cluster, error) {
	cluster, err := cs.ch.MDBCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return ClusterToGRPC(cluster, cs.saltEnvMapper), nil
}

func (cs *ClusterService) List(ctx context.Context, req *chv1.ListClustersRequest) (*chv1.ListClustersResponse, error) {
	var pageToken clustermodels.ClusterPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	pageSize := pagination.SanePageSize(req.GetPageSize())

	clusters, err := cs.ch.MDBClusters(ctx, req.GetFolderId(), pageSize, pageToken)
	if err != nil {
		return nil, err
	}

	clusterPageToken := chmodels.NewMDBClusterPageToken(clusters, pageSize)

	nextPageToken, err := api.BuildPageTokenToGRPC(clusterPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListClustersResponse{
		Clusters:      ClustersToGRPC(clusters, cs.saltEnvMapper),
		NextPageToken: nextPageToken,
	}, nil
}

func (cs *ClusterService) Create(ctx context.Context, req *chv1.CreateClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}
	specs, err := UserSpecsFromGRPC(req.GetUserSpecs())
	if err != nil {
		return nil, err
	}

	hostSpecs, err := HostSpecsFromGRPC(req.GetHostSpecs())
	if err != nil {
		return nil, err
	}

	configSpec, err := ClusterConfigSpecFromGRPC(req.GetConfigSpec())
	if err != nil {
		return nil, err
	}

	window, err := MaintenanceWindowFromGRPC(req.GetMaintenanceWindow())
	if err != nil {
		return nil, err
	}

	op, err := cs.ch.CreateMDBCluster(
		ctx,
		clickhouse.CreateMDBClusterArgs{
			FolderExtID:        req.GetFolderId(),
			Name:               req.GetName(),
			Environment:        env,
			ClusterSpec:        configSpec,
			DatabaseSpecs:      DatabaseSpecsFromGRPC(req.GetDatabaseSpecs()),
			UserSpecs:          specs,
			HostSpecs:          hostSpecs,
			NetworkID:          req.GetNetworkId(),
			Description:        req.GetDescription(),
			Labels:             req.GetLabels(),
			ShardName:          req.GetShardName(),
			MaintenanceWindow:  window,
			ServiceAccountID:   req.GetServiceAccountId(),
			SecurityGroupIDs:   req.GetSecurityGroupIds(),
			DeletionProtection: req.GetDeletionProtection(),
		})
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Restore(ctx context.Context, req *chv1.RestoreClusterRequest) (*operation.Operation, error) {
	reqEnv := req.GetEnvironment()
	if reqEnv == chv1.Cluster_ENVIRONMENT_UNSPECIFIED {
		reqEnv = chv1.Cluster_PRODUCTION
	}
	env, err := cs.saltEnvMapper.FromGRPC(int64(reqEnv))
	if err != nil {
		return nil, err
	}

	hostSpecs, err := HostSpecsFromGRPC(req.GetHostSpecs())
	if err != nil {
		return nil, err
	}

	configSpec, err := RestoreClusterConfigSpecFromGRPC(req.GetConfigSpec())
	if err != nil {
		return nil, err
	}

	window, err := MaintenanceWindowFromGRPC(req.GetMaintenanceWindow())
	if err != nil {
		return nil, err
	}

	backupIDs := []string{req.GetBackupId()}
	backupIDs = append(backupIDs, req.GetAdditionalBackupIds()...)

	op, err := cs.ch.RestoreMDBCluster(
		ctx,
		backupIDs,
		clickhouse.RestoreMDBClusterArgs{
			FolderExtID:       grpcapi.OptionalStringFromGRPC(req.GetFolderId()),
			Name:              req.GetName(),
			Environment:       env,
			ClusterSpec:       configSpec,
			HostSpecs:         hostSpecs,
			NetworkID:         req.GetNetworkId(),
			Description:       req.GetDescription(),
			Labels:            req.GetLabels(),
			MaintenanceWindow: window,
			ServiceAccountID:  req.GetServiceAccountId(),
			SecurityGroupIDs:  req.GetSecurityGroupIds(),
		})
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Update(ctx context.Context, req *chv1.UpdateClusterRequest) (*operation.Operation, error) {
	args, err := ModifyClusterArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := cs.ch.UpdateMDBCluster(ctx, args)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Delete(ctx context.Context, req *chv1.DeleteClusterRequest) (*operation.Operation, error) {
	op, err := cs.ch.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) AddZookeeper(ctx context.Context, req *chv1.AddClusterZookeeperRequest) (*operation.Operation, error) {
	hostSpecs, hsError := HostSpecsFromGRPC(req.GetHostSpecs())
	if hsError != nil {
		return nil, hsError
	}

	var resources models.ClusterResourcesSpec
	if res := req.GetResources(); res != nil {
		resources = grpcapi.ResourcesFromGRPC(req.GetResources(), grpcapi.AllPaths())
	}

	op, err := cs.ch.AddZookeeper(ctx, req.GetClusterId(), resources, hostSpecs)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Start(ctx context.Context, req *chv1.StartClusterRequest) (*operation.Operation, error) {
	op, err := cs.ch.StartCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Stop(ctx context.Context, req *chv1.StopClusterRequest) (*operation.Operation, error) {
	op, err := cs.ch.StopCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Move(ctx context.Context, req *chv1.MoveClusterRequest) (*operation.Operation, error) {
	op, err := cs.ch.MoveCluster(ctx, req.GetClusterId(), req.GetDestinationFolderId())
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Backup(ctx context.Context, req *chv1.BackupClusterRequest) (*operation.Operation, error) {
	op, err := cs.ch.BackupCluster(ctx, req.GetClusterId(), optional.String{})
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *chv1.ListClusterLogsRequest) (*chv1.ListClusterLogsResponse, error) {
	lst, err := ListLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return nil, err
	}

	logsList, token, more, err := cs.cs.ListLogs(ctx, lst, req)
	if err != nil {
		return nil, err
	}

	var respLogs []*chv1.LogRecord
	for _, l := range logsList {
		respLogs = append(respLogs, &chv1.LogRecord{Timestamp: grpcapi.TimeToGRPC(l.Timestamp), Message: l.Message})
	}

	resp := &chv1.ListClusterLogsResponse{Logs: respLogs}
	if more || req.GetAlwaysNextPageToken() {
		resp.NextPageToken = api.PagingTokenToGRPC(token)
	}

	return resp, nil
}

func (cs *ClusterService) StreamLogs(req *chv1.StreamClusterLogsRequest, stream chv1.ClusterService_StreamLogsServer) error {
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
				&chv1.StreamLogRecord{
					Record: &chv1.LogRecord{
						Timestamp: grpcapi.TimeToGRPC(l.Timestamp),
						Message:   l.Message,
					},
					NextRecordToken: api.PagingTokenToGRPC(l.NextMessageToken),
				},
			)
		},
	)
}

func (cs *ClusterService) ListOperations(ctx context.Context, req *chv1.ListClusterOperationsRequest) (*chv1.ListClusterOperationsResponse, error) {
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

	// TODO next page token

	return &chv1.ListClusterOperationsResponse{Operations: opsResponse}, nil
}

func (cs *ClusterService) ListBackups(ctx context.Context, req *chv1.ListClusterBackupsRequest) (*chv1.ListClusterBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), pageToken)
	if err != nil {
		return &chv1.ListClusterBackupsResponse{}, err
	}

	backups, backupPageToken, err := cs.ch.ClusterBackups(ctx, req.ClusterId, pageToken, req.GetPageSize())
	if err != nil {
		return &chv1.ListClusterBackupsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(backupPageToken, false)
	if err != nil {
		return &chv1.ListClusterBackupsResponse{}, err
	}

	return &chv1.ListClusterBackupsResponse{
		Backups:       BackupsToGRPC(backups),
		NextPageToken: nextPageToken,
	}, nil
}

func (cs *ClusterService) ListHosts(ctx context.Context, req *chv1.ListClusterHostsRequest) (*chv1.ListClusterHostsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &chv1.ListClusterHostsResponse{}, err
	}

	hostsList, hostPageToken, err := cs.ch.ListHosts(ctx, req.ClusterId, req.GetPageSize(), pageToken.Offset)
	if err != nil {
		return &chv1.ListClusterHostsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(hostPageToken, false)
	if err != nil {
		return &chv1.ListClusterHostsResponse{}, err
	}

	return &chv1.ListClusterHostsResponse{
		Hosts:         HostsToGRPC(hostsList),
		NextPageToken: nextPageToken,
	}, err
}

func (cs *ClusterService) AddHosts(ctx context.Context, req *chv1.AddClusterHostsRequest) (*operation.Operation, error) {
	hostSpecs, err := HostSpecsFromGRPC(req.GetHostSpecs())
	if err != nil {
		return &operation.Operation{}, err
	}

	op, err := cs.ch.AddHosts(ctx, req.GetClusterId(), hostSpecs, req.GetCopySchema().GetValue())
	if err != nil {
		return &operation.Operation{}, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) UpdateHosts(ctx context.Context, req *chv1.UpdateClusterHostsRequest) (*operation.Operation, error) {
	updateHostSpecs, err := UpdateHostSpecsFromGRPC(req.GetUpdateHostSpecs())

	if err != nil {
		return &operation.Operation{}, err
	}

	op, err := cs.ch.UpdateHosts(ctx, req.GetClusterId(), updateHostSpecs)

	if err != nil {
		return &operation.Operation{}, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) DeleteHosts(context context.Context, req *chv1.DeleteClusterHostsRequest) (*operation.Operation, error) {
	op, err := cs.ch.DeleteHosts(context, req.GetClusterId(), req.GetHostNames())
	if err != nil {
		return &operation.Operation{}, err
	}

	return grpcapi.OperationToGRPC(context, op, cs.cs.L)
}

func (cs *ClusterService) GetShard(ctx context.Context, req *chv1.GetClusterShardRequest) (*chv1.Shard, error) {
	shard, err := cs.ch.GetShard(ctx, req.GetClusterId(), req.GetShardName())
	if err != nil {
		return nil, err
	}

	return ShardToGRPC(shard), nil
}

func (cs *ClusterService) ListShards(ctx context.Context, req *chv1.ListClusterShardsRequest) (*chv1.ListClusterShardsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	shards, shardsPageToken, err := cs.ch.ListShards(ctx, req.GetClusterId(), req.GetPageSize(), pageToken.Offset)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(shardsPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListClusterShardsResponse{
		Shards:        ShardsToGRPC(shards),
		NextPageToken: nextPageToken,
	}, nil
}

func (cs *ClusterService) AddShard(ctx context.Context, req *chv1.AddClusterShardRequest) (*operation.Operation, error) {
	args, err := CreateShardArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := cs.ch.AddShard(ctx, req.GetClusterId(), args)
	if err != nil {
		return &operation.Operation{}, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) DeleteShard(ctx context.Context, req *chv1.DeleteClusterShardRequest) (*operation.Operation, error) {
	op, err := cs.ch.DeleteShard(ctx, req.GetClusterId(), req.GetShardName())
	if err != nil {
		return &operation.Operation{}, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) GetShardGroup(ctx context.Context, in *chv1.GetClusterShardGroupRequest) (*chv1.ShardGroup, error) {
	shardGroup, err := cs.ch.ShardGroup(ctx, in.GetClusterId(), in.GetShardGroupName())
	if err != nil {
		return nil, err
	}

	return ShardGroupToGRPC(shardGroup), nil
}

func (cs *ClusterService) ListShardGroups(ctx context.Context, req *chv1.ListClusterShardGroupsRequest) (*chv1.ListClusterShardGroupsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &chv1.ListClusterShardGroupsResponse{}, err
	}

	shardGroups, shardGroupPageToken, err := cs.ch.ShardGroups(ctx, req.GetClusterId(), req.GetPageSize(), pageToken.Offset)
	if err != nil {
		return nil, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(shardGroupPageToken, false)
	if err != nil {
		return &chv1.ListClusterShardGroupsResponse{}, err
	}

	return &chv1.ListClusterShardGroupsResponse{
		ShardGroups:   ShardGroupsToGRPC(shardGroups),
		NextPageToken: nextPageToken,
	}, nil
}

func (cs *ClusterService) CreateShardGroup(ctx context.Context, in *chv1.CreateClusterShardGroupRequest) (*operation.Operation, error) {
	op, err := cs.ch.CreateShardGroup(ctx, chmodels.ShardGroup{
		ClusterID:   in.GetClusterId(),
		Name:        in.GetShardGroupName(),
		Description: in.GetDescription(),
		ShardNames:  in.GetShardNames()})
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) UpdateShardGroup(ctx context.Context, in *chv1.UpdateClusterShardGroupRequest) (*operation.Operation, error) {
	groupUpdate, err := ShardGroupUpdateFromGRPC(in)
	if err != nil {
		return nil, err
	}
	op, err := cs.ch.UpdateShardGroup(ctx, groupUpdate)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) DeleteShardGroup(ctx context.Context, in *chv1.DeleteClusterShardGroupRequest) (*operation.Operation, error) {
	op, err := cs.ch.DeleteShardGroup(ctx, in.GetClusterId(), in.GetShardGroupName())
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) CreateExternalDictionary(ctx context.Context, req *chv1.CreateClusterExternalDictionaryRequest) (*operation.Operation, error) {
	dict, err := DictionaryFromGRPC(req.GetExternalDictionary())
	if err != nil {
		return nil, err
	}

	op, err := cs.ch.CreateExternalDictionary(ctx, req.GetClusterId(), dict)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) UpdateExternalDictionary(ctx context.Context, req *chv1.UpdateClusterExternalDictionaryRequest) (*operation.Operation, error) {
	dict, err := DictionaryFromGRPC(req.GetExternalDictionary())
	if err != nil {
		return nil, err
	}

	op, err := cs.ch.UpdateExternalDictionary(ctx, req.GetClusterId(), dict)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) DeleteExternalDictionary(ctx context.Context, req *chv1.DeleteClusterExternalDictionaryRequest) (*operation.Operation, error) {
	op, err := cs.ch.DeleteExternalDictionary(ctx, req.GetClusterId(), req.GetExternalDictionaryName())
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) RescheduleMaintenance(ctx context.Context, req *chv1.RescheduleMaintenanceRequest) (*operation.Operation, error) {
	rescheduleType, err := RescheduleTypeFromGRPC(req.GetRescheduleType())
	if err != nil {
		return nil, err
	}

	op, err := cs.ch.RescheduleMaintenance(ctx, req.GetClusterId(), rescheduleType, grpcapi.OptionalTimeFromGRPC(req.GetDelayedUntil()))
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ch, cs.saltEnvMapper, cs.cs.L)
}
