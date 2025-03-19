package sqlserver

import (
	"context"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/slices"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	ssv1.UnimplementedClusterServiceServer

	cs            *grpcapi.ClusterService
	ss            sqlserver.SQLServer
	ops           common.Operations
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ ssv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, ss sqlserver.SQLServer, ops common.Operations, saltEnvsCfg logic.SaltEnvsConfig) *ClusterService {
	return &ClusterService{
		cs:  cs,
		ss:  ss,
		ops: ops,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(ssv1.Cluster_PRODUCTION),
			int64(ssv1.Cluster_PRESTABLE),
			int64(ssv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *ssv1.GetClusterRequest) (*ssv1.Cluster, error) {
	cluster, err := cs.ss.Cluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return clusterToGRPC(cluster, cs.saltEnvMapper), nil
}

func (cs *ClusterService) List(ctx context.Context, req *ssv1.ListClustersRequest) (*ssv1.ListClustersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	clusters, err := cs.ss.Clusters(ctx, req.GetFolderId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &ssv1.ListClustersResponse{Clusters: clustersToGRPC(clusters, cs.saltEnvMapper)}, nil
}

func (cs *ClusterService) Create(ctx context.Context, req *ssv1.CreateClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}
	configSpec, err := clusterConfigSpecFromGRPC(req.GetConfigSpec(), grpcapi.AllPaths())
	if err != nil {
		return nil, err
	}
	op, err := cs.ss.CreateCluster(
		ctx,
		sqlserver.CreateClusterArgs{
			NewClusterArgs: sqlserver.NewClusterArgs{
				FolderExtID:        req.GetFolderId(),
				Name:               req.GetName(),
				Description:        req.GetDescription(),
				Labels:             req.GetLabels(),
				Environment:        env,
				NetworkID:          req.GetNetworkId(),
				SecurityGroupIDs:   req.GetSecurityGroupIds(),
				ClusterConfigSpec:  configSpec,
				HostSpecs:          hostSpecsFromGRPC(req.GetHostSpecs()),
				DeletionProtection: req.GetDeletionProtection(),
				HostGroupIDs:       slices.DedupStrings(req.GetHostGroupIds()),
				ServiceAccountID:   req.GetServiceAccountId(),
			},
			SQLCollation:  req.GetSqlcollation(),
			UserSpecs:     userSpecsFromGRPC(req.GetUserSpecs()),
			DatabaseSpecs: databaseSpecsFromGRPC(req.GetDatabaseSpecs()),
		})
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Backup(ctx context.Context, req *ssv1.BackupClusterRequest) (*operation.Operation, error) {
	op, err := cs.ss.BackupCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Restore(ctx context.Context, req *ssv1.RestoreClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}
	configSpec, err := clusterConfigSpecFromGRPC(req.GetConfigSpec(), grpcapi.AllPaths())
	if err != nil {
		return nil, err
	}

	op, err := cs.ss.RestoreCluster(
		ctx,
		sqlserver.RestoreClusterArgs{
			NewClusterArgs: sqlserver.NewClusterArgs{
				FolderExtID:        req.GetFolderId(),
				Name:               req.GetName(),
				Description:        req.GetDescription(),
				Labels:             req.GetLabels(),
				Environment:        env,
				NetworkID:          req.GetNetworkId(),
				SecurityGroupIDs:   req.GetSecurityGroupIds(),
				ClusterConfigSpec:  configSpec,
				HostSpecs:          hostSpecsFromGRPC(req.GetHostSpecs()),
				DeletionProtection: req.GetDeletionProtection(),
				HostGroupIDs:       slices.DedupStrings(req.GetHostGroupIds()),
				ServiceAccountID:   req.GetServiceAccountId(),
			},
			BackupID: req.GetBackupId(),
			Time:     grpcapi.TimeFromGRPC(req.GetTime()),
		})
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Update(ctx context.Context, req *ssv1.UpdateClusterRequest) (*operation.Operation, error) {
	args, err := clusterModifyArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := cs.ss.ModifyCluster(ctx, args)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.ss, cs.saltEnvMapper, cs.cs.L)
	//return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Delete(ctx context.Context, req *ssv1.DeleteClusterRequest) (*operation.Operation, error) {
	op, err := cs.ss.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *ssv1.ListClusterLogsRequest) (*ssv1.ListClusterLogsResponse, error) {
	return nil, semerr.NotImplemented("not implemented")
}

func (cs *ClusterService) ListOperations(ctx context.Context, req *ssv1.ListClusterOperationsRequest) (*ssv1.ListClusterOperationsResponse, error) {
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
	return &ssv1.ListClusterOperationsResponse{Operations: opsResponse}, nil
}

func (cs *ClusterService) ListHosts(ctx context.Context, req *ssv1.ListClusterHostsRequest) (*ssv1.ListClusterHostsResponse, error) {
	hosts, err := cs.ss.ListHosts(ctx, req.ClusterId)
	if err != nil {
		return nil, err
	}
	return &ssv1.ListClusterHostsResponse{
		Hosts: hostsToGRPC(hosts),
	}, err
}

func (cs *ClusterService) ListBackups(ctx context.Context, req *ssv1.ListClusterBackupsRequest) (*ssv1.ListClusterBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}
	backups, nextPageToken, err := cs.ss.ListBackups(ctx, req.ClusterId, pageToken, req.PageSize)
	if err != nil {
		return nil, err
	}
	nextToken, err := api.BuildPageTokenToGRPC(nextPageToken, false)
	if err != nil {
		return nil, err
	}
	return &ssv1.ListClusterBackupsResponse{
		Backups:       backupsToGRPC(backups),
		NextPageToken: nextToken,
	}, err
}

func (cs *ClusterService) Start(ctx context.Context, req *ssv1.StartClusterRequest) (*operation.Operation, error) {
	op, err := cs.ss.StartCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Stop(ctx context.Context, req *ssv1.StopClusterRequest) (*operation.Operation, error) {
	op, err := cs.ss.StopCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) StartFailover(ctx context.Context, req *ssv1.StartClusterFailoverRequest) (*operation.Operation, error) {
	op, err := cs.ss.StartFailover(ctx, req.GetClusterId(), req.GetHostName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) UpdateHosts(ctx context.Context, req *ssv1.UpdateClusterHostsRequest) (*operation.Operation, error) {
	updateHostSpecs, err := UpdateHostSpecsFromGRPC(req.GetUpdateHostSpecs())

	if err != nil {
		return &operation.Operation{}, err
	}

	op, err := cs.ss.UpdateHosts(ctx, req.GetClusterId(), updateHostSpecs)

	if err != nil {
		return &operation.Operation{}, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}
