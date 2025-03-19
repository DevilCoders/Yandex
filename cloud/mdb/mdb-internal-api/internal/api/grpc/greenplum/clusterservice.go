package greenplum

import (
	"context"

	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/gpmodels"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/slices"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	gpv1.UnimplementedClusterServiceServer

	cs            *grpcapi.ClusterService
	gp            greenplum.Greenplum
	ops           common.Operations
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ gpv1.ClusterServiceServer = &ClusterService{}
var (
	mapListLogsServiceTypeToGRPC = map[logs.ServiceType]gpv1.ListClusterLogsRequest_ServiceType{
		logs.ServiceTypeGreenplum:       gpv1.ListClusterLogsRequest_GREENPLUM,
		logs.ServiceTypeGreenplumPooler: gpv1.ListClusterLogsRequest_GREENPLUM_POOLER,
	}
	mapListLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapListLogsServiceTypeToGRPC).(map[gpv1.ListClusterLogsRequest_ServiceType]logs.ServiceType)
)

func NewClusterService(cs *grpcapi.ClusterService, gp greenplum.Greenplum, ops common.Operations, saltEnvsCfg logic.SaltEnvsConfig) *ClusterService {
	return &ClusterService{
		cs:  cs,
		gp:  gp,
		ops: ops,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(gpv1.Cluster_PRODUCTION),
			int64(gpv1.Cluster_PRESTABLE),
			int64(gpv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *gpv1.GetClusterRequest) (*gpv1.Cluster, error) {
	cluster, err := cs.gp.Cluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return clusterToGRPC(cluster, cs.saltEnvMapper)
}

func (cs *ClusterService) List(ctx context.Context, req *gpv1.ListClustersRequest) (*gpv1.ListClustersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	clusters, err := cs.gp.Clusters(ctx, req.GetFolderId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	cls, err := clustersToGRPC(clusters, cs.saltEnvMapper)
	if err != nil {
		return nil, err
	}

	return &gpv1.ListClustersResponse{Clusters: cls}, nil
}

func (cs *ClusterService) Create(ctx context.Context, req *gpv1.CreateClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}

	config, err := clusterConfigSpecFromGRPC(req.GetConfig(), grpcapi.AllPaths(), req.GetMasterConfig(), req.GetSegmentConfig())
	if err != nil {
		return nil, err
	}
	configMaster, err := clusterMasterConfigSpecFromGRPC(req.GetMasterConfig(), grpcapi.AllPaths())
	if err != nil {
		return nil, err
	}
	configSegment, err := clusterSegmentConfigSpecFromGRPC(req.GetSegmentConfig(), grpcapi.AllPaths())
	if err != nil {
		return nil, err
	}
	configSpec, err := clusterDBConfigSpecFromGRPC(req.GetConfigSpec(), req.GetConfig(), grpcapi.AllPaths())
	if err != nil {
		return nil, err
	}
	window, err := MaintenanceWindowFromGRPC(req.GetMaintenanceWindow())
	if err != nil {
		return nil, err
	}
	op, err := cs.gp.CreateCluster(
		ctx,
		greenplum.CreateClusterArgs{
			FolderExtID:        req.GetFolderId(),
			Name:               req.GetName(),
			Description:        req.GetDescription(),
			Labels:             req.GetLabels(),
			Environment:        env,
			NetworkID:          req.GetNetworkId(),
			SecurityGroupIDs:   req.GetSecurityGroupIds(),
			DeletionProtection: req.GetDeletionProtection(),
			HostGroupIDs:       slices.DedupStrings(req.GetHostGroupIds()),

			Config:        config,
			MasterConfig:  configMaster,
			SegmentConfig: configSegment,
			ConfigSpec:    configSpec,

			MasterHostCount:  int(req.GetMasterHostCount()),
			SegmentHostCount: int(req.GetSegmentHostCount()),
			SegmentInHost:    int(req.GetSegmentInHost()),

			UserName:     req.GetUserName(),
			UserPassword: req.GetUserPassword(),

			MaintenanceWindow: window,
		})
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Update(ctx context.Context, req *gpv1.UpdateClusterRequest) (*operation.Operation, error) {
	args, err := clusterModifyArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := cs.gp.ModifyCluster(ctx, args)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.gp, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Delete(ctx context.Context, req *gpv1.DeleteClusterRequest) (*operation.Operation, error) {
	op, err := cs.gp.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func ListLogsServiceTypeFromGRPC(st gpv1.ListClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapListLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *gpv1.ListClusterLogsRequest) (*gpv1.ListClusterLogsResponse, error) {
	lst, err := ListLogsServiceTypeFromGRPC(req.GetServiceType())
	if err != nil {
		return nil, err
	}
	logsList, token, more, err := cs.cs.ListLogs(ctx, lst, req)
	if err != nil {
		return nil, err
	}

	var respLogs []*gpv1.LogRecord
	for _, l := range logsList {
		respLogs = append(respLogs, &gpv1.LogRecord{Timestamp: grpcapi.TimeToGRPC(l.Timestamp), Message: l.Message})
	}

	resp := &gpv1.ListClusterLogsResponse{Logs: respLogs}
	if more || req.GetAlwaysNextPageToken() {
		resp.NextPageToken = api.PagingTokenToGRPC(token)
	}
	return resp, nil
}

func (cs *ClusterService) ListOperations(ctx context.Context, req *gpv1.ListClusterOperationsRequest) (*gpv1.ListClusterOperationsResponse, error) {
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
	return &gpv1.ListClusterOperationsResponse{Operations: opsResponse}, nil
}

func (cs *ClusterService) StreamLogs(req *gpv1.StreamClusterLogsRequest, stream gpv1.ClusterService_StreamLogsServer) error {
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
				&gpv1.StreamLogRecord{
					Record: &gpv1.LogRecord{
						Timestamp: grpcapi.TimeToGRPC(l.Timestamp),
						Message:   l.Message,
					},
					NextRecordToken: api.PagingTokenToGRPC(l.NextMessageToken),
				},
			)
		},
	)
}

func (cs *ClusterService) ListMasterHosts(ctx context.Context, req *gpv1.ListClusterHostsRequest) (*gpv1.ListClusterHostsResponse, error) {
	hosts, err := cs.gp.ListMasterHosts(ctx, req.ClusterId)
	if err != nil {
		return nil, err
	}
	return &gpv1.ListClusterHostsResponse{
		Hosts: hostsToGRPC(hosts),
	}, err
}

func (cs *ClusterService) ListSegmentHosts(ctx context.Context, req *gpv1.ListClusterHostsRequest) (*gpv1.ListClusterHostsResponse, error) {
	hosts, err := cs.gp.ListSegmentHosts(ctx, req.ClusterId)
	if err != nil {
		return nil, err
	}
	return &gpv1.ListClusterHostsResponse{
		Hosts: hostsToGRPC(hosts),
	}, err
}

func (cs *ClusterService) Start(ctx context.Context, req *gpv1.StartClusterRequest) (*operation.Operation, error) {
	op, err := cs.gp.StartCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Stop(ctx context.Context, req *gpv1.StopClusterRequest) (*operation.Operation, error) {
	op, err := cs.gp.StopCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Move(ctx context.Context, req *gpv1.MoveClusterRequest) (*operation.Operation, error) {
	return nil, semerr.NotImplemented("not implemented")
}

func (cs *ClusterService) RescheduleMaintenance(ctx context.Context, req *gpv1.RescheduleMaintenanceRequest) (*operation.Operation, error) {
	rescheduleType, err := RescheduleTypeFromGRPC(req.GetRescheduleType())
	if err != nil {
		return nil, err
	}

	cid := req.GetClusterId()
	delayedUntil := req.GetDelayedUntil()
	optionalDelayedUntil := optional.Time{Time: delayedUntil.AsTime(), Valid: delayedUntil.CheckValid() == nil}

	op, err := cs.gp.RescheduleMaintenance(ctx, cid, rescheduleType, optionalDelayedUntil)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) AddHosts(ctx context.Context, req *gpv1.AddClusterHostsRequest) (*operation.Operation, error) {
	args, err := addHostsArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := cs.gp.AddHosts(ctx, args)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.gp, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) ListBackups(ctx context.Context, req *gpv1.ListClusterBackupsRequest) (*gpv1.ListClusterBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), pageToken)
	if err != nil {
		return &gpv1.ListClusterBackupsResponse{}, err
	}

	backups, backupPageToken, err := cs.gp.ClusterBackups(ctx, req.ClusterId, pageToken, req.GetPageSize())
	if err != nil {
		return &gpv1.ListClusterBackupsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(backupPageToken, false)
	if err != nil {
		return &gpv1.ListClusterBackupsResponse{}, err
	}

	return &gpv1.ListClusterBackupsResponse{
		Backups:       BackupsToGRPC(backups),
		NextPageToken: nextPageToken,
	}, nil
}

func (cs *ClusterService) Restore(ctx context.Context, req *gpv1.RestoreClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}

	config, err := clusterRestoreConfigSpecFromGRPC(req.GetConfig())
	if err != nil {
		return nil, err
	}

	masterResources, err := clusterResourcesGRPC(req.GetMasterResources())
	if err != nil {
		return nil, err
	}
	segmentResources, err := clusterResourcesGRPC(req.GetSegmentResources())
	if err != nil {
		return nil, err
	}

	window, err := MaintenanceWindowFromGRPC(req.GetMaintenanceWindow())
	if err != nil {
		return nil, err
	}
	op, err := cs.gp.RestoreCluster(
		ctx,
		greenplum.CreateClusterArgs{
			FolderExtID:        req.GetFolderId(),
			Name:               req.GetName(),
			Description:        req.GetDescription(),
			Labels:             req.GetLabels(),
			Environment:        env,
			NetworkID:          req.GetNetworkId(),
			SecurityGroupIDs:   req.GetSecurityGroupIds(),
			DeletionProtection: req.GetDeletionProtection(),
			HostGroupIDs:       slices.DedupStrings(req.GetHostGroupIds()),
			Config:             config,
			MasterConfig: gpmodels.MasterSubclusterConfigSpec{
				Resources: masterResources,
			},
			SegmentConfig: gpmodels.SegmentSubclusterConfigSpec{
				Resources: segmentResources,
			},
			MaintenanceWindow: window,
		},
		req.GetBackupId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) GetRestoreHints(ctx context.Context, req *gpv1.GetRestoreHintsRequest) (*gpv1.RestoreHints, error) {
	hints, err := cs.gp.RestoreHints(ctx, req.BackupId)
	if err != nil {
		return nil, err
	}
	return &gpv1.RestoreHints{
		Environment: gpv1.Cluster_Environment(cs.saltEnvMapper.ToGRPC(hints.Environment)),
		Version:     hints.Version,
		NetworkId:   hints.NetworkID,
		Time:        grpcapi.TimeToGRPC(hints.Time),
		MasterResources: &gpv1.RestoreResources{
			ResourcePresetId: hints.MasterResources.ResourcePresetID,
			DiskSize:         hints.MasterResources.DiskSize,
		},
		SegmentResources: &gpv1.RestoreResources{
			ResourcePresetId: hints.SegmentResources.ResourcePresetID,
			DiskSize:         hints.SegmentResources.DiskSize,
		},
		MasterHostCount:  hints.MasterHostCount,
		SegmentHostCount: hints.SegmentHostCount,
		SegmentInHost:    hints.SegmentInHost,
	}, nil
}
