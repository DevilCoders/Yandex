package opensearch

import (
	"context"

	protov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/opensearch/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	protov1.UnimplementedClusterServiceServer

	cs            *grpcapi.ClusterService
	es            opensearch.OpenSearch
	ops           common.Operations
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ protov1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, es opensearch.OpenSearch, ops common.Operations, saltEnvsCfg logic.SaltEnvsConfig) *ClusterService {
	return &ClusterService{
		cs:  cs,
		es:  es,
		ops: ops,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(protov1.Cluster_PRODUCTION),
			int64(protov1.Cluster_PRESTABLE),
			int64(protov1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *protov1.GetClusterRequest) (*protov1.Cluster, error) {
	cluster, err := cs.es.Cluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return ClusterToGRPC(cluster, cs.saltEnvMapper), nil
}

func (cs *ClusterService) List(ctx context.Context, req *protov1.ListClustersRequest) (*protov1.ListClustersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	clusters, err := cs.es.Clusters(ctx, req.GetFolderId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &protov1.ListClustersResponse{Clusters: ClustersToGRPC(clusters, cs.saltEnvMapper)}, nil
}

func (cs *ClusterService) Create(ctx context.Context, req *protov1.CreateClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}
	hostSpec, err := HostsFromGRPC(req.HostSpecs)
	if err != nil {
		return nil, err
	}
	configSpec, err := ConfigFromGRPC(req.ConfigSpec, cs.es.SupportedVersions(ctx))
	if err != nil {
		return nil, err
	}
	mntWindow, err := MaintenanceWindowFromGRPC(req.MaintenanceWindow)
	if err != nil {
		return nil, semerr.InvalidInputf("bad maintenance window: %v", err)
	}

	op, err := cs.es.CreateCluster(
		ctx,
		opensearch.CreateClusterArgs{
			FolderExtID:        req.GetFolderId(),
			Name:               req.GetName(),
			Description:        req.GetDescription(),
			Labels:             req.GetLabels(),
			Environment:        env,
			NetworkID:          req.GetNetworkId(),
			SecurityGroupIDs:   req.GetSecurityGroupIds(),
			ServiceAccountID:   req.GetServiceAccountId(),
			HostSpec:           hostSpec,
			ConfigSpec:         configSpec,
			ExtensionSpecs:     ExtensionSpecsFromGRPC(req.ExtensionSpecs),
			DeletionProtection: req.GetDeletionProtection(),
			MaintenanceWindow:  mntWindow,
		})
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Update(ctx context.Context, req *protov1.UpdateClusterRequest) (*operation.Operation, error) {
	args, err := modifyClusterArgsFromGRPC(req, cs.es.SupportedVersions(ctx))
	if err != nil {
		return nil, err
	}
	op, err := cs.es.ModifyCluster(ctx, args)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.es, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Delete(ctx context.Context, req *protov1.DeleteClusterRequest) (*operation.Operation, error) {
	op, err := cs.es.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Start(ctx context.Context, req *protov1.StartClusterRequest) (*operation.Operation, error) {
	op, err := cs.es.StartCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Stop(ctx context.Context, req *protov1.StopClusterRequest) (*operation.Operation, error) {
	op, err := cs.es.StopCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Move(context.Context, *protov1.MoveClusterRequest) (*operation.Operation, error) {
	return nil, semerr.NotImplemented("not implemented")
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *protov1.ListClusterLogsRequest) (*protov1.ListClusterLogsResponse, error) {
	lst, err := ListLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return nil, err
	}

	lgs, token, more, err := cs.cs.ListLogs(ctx, lst, req)
	if err != nil {
		return nil, err
	}

	var respLogs []*protov1.LogRecord
	for _, l := range lgs {
		respLogs = append(respLogs, &protov1.LogRecord{Timestamp: grpcapi.TimeToGRPC(l.Timestamp), Message: l.Message})
	}

	resp := &protov1.ListClusterLogsResponse{Logs: respLogs}
	if more || req.GetAlwaysNextPageToken() {
		resp.NextPageToken = api.PagingTokenToGRPC(token)
	}

	return resp, nil
}

func (cs *ClusterService) StreamLogs(req *protov1.StreamClusterLogsRequest, stream protov1.ClusterService_StreamLogsServer) error {
	lst, err := StreamLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return err
	}

	return cs.cs.StreamLogs(stream.Context(), lst, req, func(l logs.Message) error {
		return stream.Send(&protov1.StreamLogRecord{
			Record: &protov1.LogRecord{
				Timestamp: grpcapi.TimeToGRPC(l.Timestamp),
				Message:   l.Message,
			},
			NextRecordToken: api.PagingTokenToGRPC(l.NextMessageToken),
		})
	})
}

func (cs *ClusterService) ListOperations(ctx context.Context,
	req *protov1.ListClusterOperationsRequest) (*protov1.ListClusterOperationsResponse, error) {
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

	return &protov1.ListClusterOperationsResponse{Operations: opsResponse}, nil
}

func (cs *ClusterService) ListHosts(ctx context.Context, req *protov1.ListClusterHostsRequest) (*protov1.ListClusterHostsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &protov1.ListClusterHostsResponse{}, err
	}

	hostList, hostPageToken, err := cs.es.ListHosts(ctx, req.ClusterId, req.PageSize, pageToken.Offset)
	if err != nil {
		return &protov1.ListClusterHostsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(hostPageToken, false)
	if err != nil {
		return &protov1.ListClusterHostsResponse{}, err
	}

	return &protov1.ListClusterHostsResponse{
		Hosts:         HostsToGRPC(hostList),
		NextPageToken: nextPageToken,
	}, err
}

func (cs *ClusterService) AddHosts(ctx context.Context, req *protov1.AddClusterHostsRequest) (*operation.Operation, error) {
	hosts, err := HostsFromGRPC(req.GetHostSpecs())
	if err != nil {
		return nil, err
	}
	op, err := cs.es.AddHosts(ctx, req.GetClusterId(), hosts)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) DeleteHosts(ctx context.Context, req *protov1.DeleteClusterHostsRequest) (*operation.Operation, error) {
	op, err := cs.es.DeleteHosts(ctx, req.GetClusterId(), req.GetHostNames())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) ListBackups(ctx context.Context, req *protov1.ListClusterBackupsRequest) (*protov1.ListClusterBackupsResponse, error) {
	var pageToken backups.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), pageToken)
	if err != nil {
		return &protov1.ListClusterBackupsResponse{}, err
	}

	backups, backupPageToken, err := cs.es.ClusterBackups(ctx, req.ClusterId, pageToken, req.GetPageSize())
	if err != nil {
		return &protov1.ListClusterBackupsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(backupPageToken, false)
	if err != nil {
		return &protov1.ListClusterBackupsResponse{}, err
	}

	return &protov1.ListClusterBackupsResponse{
		Backups:       BackupsToGRPC(backups),
		NextPageToken: nextPageToken,
	}, nil
}

func (cs *ClusterService) Backup(ctx context.Context, req *protov1.BackupClusterRequest) (*operation.Operation, error) {
	op, err := cs.es.BackupCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Restore(ctx context.Context, req *protov1.RestoreClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}
	hostSpec, err := HostsFromGRPC(req.HostSpecs)
	if err != nil {
		return nil, err
	}
	configSpec, err := ConfigFromGRPC(req.ConfigSpec, cs.es.SupportedVersions(ctx))
	if err != nil {
		return nil, err
	}
	op, err := cs.es.RestoreCluster(
		ctx,
		opensearch.RestoreClusterArgs{
			CreateClusterArgs: opensearch.CreateClusterArgs{
				FolderExtID:        req.GetFolderId(),
				Name:               req.GetName(),
				Description:        req.GetDescription(),
				Labels:             req.GetLabels(),
				Environment:        env,
				NetworkID:          req.GetNetworkId(),
				SecurityGroupIDs:   req.GetSecurityGroupIds(),
				ServiceAccountID:   req.GetServiceAccountId(),
				HostSpec:           hostSpec,
				ConfigSpec:         configSpec,
				DeletionProtection: req.GetDeletionProtection(),
				RestoreFrom:        req.GetBackupId(),
			}})
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) RescheduleMaintenance(ctx context.Context, req *protov1.RescheduleMaintenanceRequest) (*operation.Operation, error) {
	rescheduleType, err := RescheduleTypeFromGRPC(req.GetRescheduleType())
	if err != nil {
		return nil, err
	}

	op, err := cs.es.RescheduleMaintenance(ctx, req.GetClusterId(), rescheduleType, grpcapi.OptionalTimeFromGRPC(req.GetDelayedUntil()))
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}
