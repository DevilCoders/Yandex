package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/slices"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	kfv1.UnimplementedClusterServiceServer

	cs            *grpcapi.ClusterService
	kf            kafka.Kafka
	ops           common.Operations
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ kfv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, kf kafka.Kafka, ops common.Operations, saltEnvsCfg logic.SaltEnvsConfig) *ClusterService {
	return &ClusterService{
		cs:  cs,
		kf:  kf,
		ops: ops,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(kfv1.Cluster_PRODUCTION),
			int64(kfv1.Cluster_PRESTABLE),
			int64(kfv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *kfv1.GetClusterRequest) (*kfv1.Cluster, error) {
	cluster, err := cs.kf.MDBCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return ClusterWithPillarToGRPC(cluster, cs.saltEnvMapper), nil
}

func (cs *ClusterService) List(ctx context.Context, req *kfv1.ListClustersRequest) (*kfv1.ListClustersResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}

	clusters, err := cs.kf.MDBClusters(ctx, req.GetFolderId(), req.GetPageSize(), offset)
	if err != nil {
		return nil, err
	}

	return &kfv1.ListClustersResponse{Clusters: ClustersToGRPC(clusters, cs.saltEnvMapper)}, nil
}

func (cs *ClusterService) Create(ctx context.Context, req *kfv1.CreateClusterRequest) (*operation.Operation, error) {
	env, err := cs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}

	version := req.GetConfigSpec().GetVersion()
	if version != "" {
		modelVersion, err := kfmodels.FindVersion(version)
		if err != nil {
			return &operation.Operation{}, err
		}
		if modelVersion.Deprecated {
			return &operation.Operation{}, semerr.InvalidInputf("version %s is deprecated", version)
		}
	}

	configSpec, err := configSpecFromGRPC(req.GetConfigSpec())
	if err != nil {
		return nil, err
	}

	window, err := MaintenanceWindowFromGRPC(req.GetMaintenanceWindow())
	if err != nil {
		return nil, err
	}
	// TODO refactor: replace deprecated method DedupStrings with method Dedup
	createClusterArgs := kafka.CreateMDBClusterArgs{
		FolderExtID:        req.GetFolderId(),
		Name:               req.GetName(),
		Description:        req.GetDescription(),
		Labels:             req.GetLabels(),
		Environment:        env,
		UserSpecs:          UserSpecsFromGRPC(req.GetUserSpecs()),
		TopicSpecs:         TopicSpecsFromGRPC(req.GetTopicSpecs()),
		NetworkID:          req.GetNetworkId(),
		SubnetID:           req.GetSubnetId(),
		ConfigSpec:         configSpec,
		SecurityGroupIDs:   slices.DedupStrings(req.GetSecurityGroupIds()),
		HostGroupIDs:       slices.DedupStrings(req.GetHostGroupIds()),
		DeletionProtection: req.GetDeletionProtection(),
		MaintenanceWindow:  window,
	}
	op, err := cs.kf.CreateMDBCluster(ctx, createClusterArgs)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Update(ctx context.Context, req *kfv1.UpdateClusterRequest) (*operation.Operation, error) {
	args, err := modifyClusterArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	op, err := cs.kf.ModifyMDBCluster(ctx, args)
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.kf, cs.saltEnvMapper, cs.cs.L)
}

func (cs *ClusterService) Delete(ctx context.Context, req *kfv1.DeleteClusterRequest) (*operation.Operation, error) {
	op, err := cs.kf.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func configSpecFromGRPC(spec *kfv1.ConfigSpec) (kfmodels.MDBClusterSpec, error) {
	kafkaConfig, configVersion := KafkaConfigFromGRPC(spec.GetKafka())
	if err := kafkaConfig.Validate(); err != nil {
		return kfmodels.MDBClusterSpec{}, err
	}
	version := spec.GetVersion()
	if configVersion == kfmodels.СonfigVersion3 {
		if version == "" {
			configVersion = kfmodels.DefaultVersion3
			version = kfmodels.DefaultVersion3
		} else if kfmodels.IsSupportedVersion3x(version) {
			configVersion = version
		} else {
			return kfmodels.MDBClusterSpec{}, semerr.InvalidInputf(
				"the config version \"%s\" does not match the cluster version \"%s\". Supported versions: %v.", configVersion, version, kfmodels.AllValidKafkaVersions3x)
		}
	}
	if version == "" {
		version = configVersion
	} else if version != configVersion && configVersion != "" {
		return kfmodels.MDBClusterSpec{}, semerr.InvalidInputf(
			"the config version %s does not match the cluster version %s", configVersion, version)
	}
	ret := kfmodels.MDBClusterSpec{
		Version:         version,
		ZoneID:          slices.DedupStrings(spec.GetZoneId()),
		BrokersCount:    spec.GetBrokersCount().GetValue(),
		AssignPublicIP:  spec.GetAssignPublicIp(),
		UnmanagedTopics: spec.GetUnmanagedTopics(),
		SchemaRegistry:  spec.GetSchemaRegistry(),
		Kafka: kfmodels.KafkaConfigSpec{
			Resources: resourcesFromSpec(spec.GetKafka().GetResources()),
			Config:    kafkaConfig,
		},
		ZooKeeper: kfmodels.ZookeperConfigSpec{
			Resources: resourcesFromSpec(spec.GetZookeeper().GetResources()),
		},
		Access: kfmodels.Access{
			DataTransfer: optional.NewBool(spec.Access.GetDataTransfer()),
			WebSQL:       optional.NewBool(spec.Access.GetWebSql()),
			Serverless:   optional.NewBool(spec.Access.GetServerless()),
		},
	}

	return ret, nil
}

func resourcesFromSpec(resources *kfv1.Resources) models.ClusterResources {
	return models.ClusterResources{
		ResourcePresetExtID: resources.GetResourcePresetId(),
		DiskSize:            resources.GetDiskSize(),
		DiskTypeExtID:       resources.GetDiskTypeId(),
	}
}

func (cs *ClusterService) ListHosts(ctx context.Context, req *kfv1.ListClusterHostsRequest) (*kfv1.ListClusterHostsResponse, error) {
	offset, _, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}
	hosts, err := cs.kf.ListHosts(ctx, req.GetClusterId(), req.GetPageSize(), offset)
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

func (cs *ClusterService) ListOperations(ctx context.Context, req *kfv1.ListClusterOperationsRequest) (*kfv1.ListClusterOperationsResponse, error) {
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

	return &kfv1.ListClusterOperationsResponse{Operations: opsResponse}, nil
}

func (cs *ClusterService) Start(ctx context.Context, req *kfv1.StartClusterRequest) (*operation.Operation, error) {
	op, err := cs.kf.StartCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Stop(ctx context.Context, req *kfv1.StopClusterRequest) (*operation.Operation, error) {
	op, err := cs.kf.StopCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) ListLogs(ctx context.Context, req *kfv1.ListClusterLogsRequest) (*kfv1.ListClusterLogsResponse, error) {
	logsList, token, more, err := cs.cs.ListLogs(ctx, logs.ServiceTypeKafka, req)
	if err != nil {
		return nil, err
	}

	var respLogs []*kfv1.LogRecord
	for _, l := range logsList {
		respLogs = append(respLogs, &kfv1.LogRecord{Timestamp: grpcapi.TimeToGRPC(l.Timestamp), Message: l.Message})
	}

	resp := &kfv1.ListClusterLogsResponse{Logs: respLogs}
	if more || req.GetAlwaysNextPageToken() {
		resp.NextPageToken = api.PagingTokenToGRPC(token)
	}

	return resp, nil
}

func (cs *ClusterService) StreamLogs(req *kfv1.StreamClusterLogsRequest, stream kfv1.ClusterService_StreamLogsServer) error {
	return cs.cs.StreamLogs(
		stream.Context(),
		logs.ServiceTypeKafka,
		req,
		func(l logs.Message) error {
			return stream.Send(
				&kfv1.StreamLogRecord{
					Record: &kfv1.LogRecord{
						Timestamp: grpcapi.TimeToGRPC(l.Timestamp),
						Message:   l.Message,
					},
					NextRecordToken: api.PagingTokenToGRPC(l.NextMessageToken),
				},
			)
		},
	)
}

func (cs *ClusterService) Move(ctx context.Context, req *kfv1.MoveClusterRequest) (*operation.Operation, error) {
	op, err := cs.kf.MoveCluster(ctx, req.GetClusterId(), req.GetDestinationFolderId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) RescheduleMaintenance(ctx context.Context, req *kfv1.RescheduleMaintenanceRequest) (*operation.Operation, error) {
	rescheduleType, err := RescheduleTypeFromGRPC(req.GetRescheduleType())
	if err != nil {
		return nil, err
	}

	op, err := cs.kf.RescheduleMaintenance(ctx, req.GetClusterId(), rescheduleType,
		grpcapi.OptionalTimeFromGRPC(req.GetDelayedUntil()))
	if err != nil {
		return nil, err
	}

	return operationToGRPC(ctx, op, cs.kf, cs.saltEnvMapper, cs.cs.L)
}
