package datacloudgrpc

import (
	"context"
	"encoding/json"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/ptypes/wrappers"

	consolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ResourcesGRPC interface {
	GetResourcePresetId() string
	GetDiskSize() *wrappers.Int64Value
}

func ResourcesFromGRPC(resSpec ResourcesGRPC) models.ClusterResourcesSpec {
	cr := models.ClusterResourcesSpec{}
	if resSpec == nil {
		return cr
	}
	if diskSize := resSpec.GetDiskSize(); diskSize != nil {
		cr.DiskSize.Set(diskSize.GetValue())
	}
	if resSpec.GetResourcePresetId() != "" {
		cr.ResourcePresetExtID.Set(resSpec.GetResourcePresetId())
	}
	return cr
}

var (
	statusToGRCP = map[clusters.Status]apiv1.ClusterStatus{
		clusters.StatusCreating:             apiv1.ClusterStatus_CLUSTER_STATUS_CREATING,
		clusters.StatusCreateError:          apiv1.ClusterStatus_CLUSTER_STATUS_ERROR,
		clusters.StatusRunning:              apiv1.ClusterStatus_CLUSTER_STATUS_ALIVE,
		clusters.StatusModifying:            apiv1.ClusterStatus_CLUSTER_STATUS_UPDATING,
		clusters.StatusModifyError:          apiv1.ClusterStatus_CLUSTER_STATUS_ERROR,
		clusters.StatusStopping:             apiv1.ClusterStatus_CLUSTER_STATUS_STOPPING,
		clusters.StatusStopped:              apiv1.ClusterStatus_CLUSTER_STATUS_STOPPED,
		clusters.StatusStopError:            apiv1.ClusterStatus_CLUSTER_STATUS_ERROR,
		clusters.StatusStarting:             apiv1.ClusterStatus_CLUSTER_STATUS_STARTING,
		clusters.StatusStartError:           apiv1.ClusterStatus_CLUSTER_STATUS_ERROR,
		clusters.StatusMaintainingOffline:   apiv1.ClusterStatus_CLUSTER_STATUS_UPDATING,
		clusters.StatusMaintainOfflineError: apiv1.ClusterStatus_CLUSTER_STATUS_ERROR,
	}

	healthToGRPC = map[clusters.Health]apiv1.ClusterStatus{
		clusters.HealthDead:     apiv1.ClusterStatus_CLUSTER_STATUS_DEAD,
		clusters.HealthDegraded: apiv1.ClusterStatus_CLUSTER_STATUS_DEGRADED,
		clusters.HealthAlive:    apiv1.ClusterStatus_CLUSTER_STATUS_ALIVE,
	}
)

func StatusToGRPC(status clusters.Status, health clusters.Health) apiv1.ClusterStatus {
	if status == clusters.StatusRunning {
		v, ok := healthToGRPC[health]
		if !ok {
			return apiv1.ClusterStatus_CLUSTER_STATUS_UNKNOWN
		}
		return v
	}

	v, ok := statusToGRCP[status]
	if !ok {
		return apiv1.ClusterStatus_CLUSTER_STATUS_INVALID
	}
	return v
}

func ResourcePresetToGRPC(preset consolemodels.ResourcePreset) *consolev1.ResourcePresetConfig {
	defaultDiskSize := preset.DefaultDiskSize.Int64
	if !preset.DefaultDiskSize.Valid {
		if len(preset.DiskSizes) > 0 {
			defaultDiskSize = preset.DiskSizes[0]
		}
	}

	return &consolev1.ResourcePresetConfig{
		ResourcePresetId: preset.ExtID,
		CpuLimit:         preset.CPULimit,
		MemoryLimit:      preset.MemoryLimit,
		MinDiskSize:      preset.DiskSizes[0],
		MaxDiskSize:      preset.DiskSizes[len(preset.DiskSizes)-1],
		DiskSizes:        preset.DiskSizes,
		DefaultDiskSize:  defaultDiskSize,
	}
}

var (
	clusterTypeToGRPC = map[clusters.Type]consolev1.ClusterType{
		clusters.TypeUnknown:    consolev1.ClusterType_CLUSTER_TYPE_INVALID,
		clusters.TypeClickHouse: consolev1.ClusterType_CLUSTER_TYPE_CLICKHOUSE,
		clusters.TypeKafka:      consolev1.ClusterType_CLUSTER_TYPE_KAFKA,
	}
)

func ClusterTypeToGRPC(clusterType clusters.Type) consolev1.ClusterType {
	v, ok := clusterTypeToGRPC[clusterType]
	if !ok {
		return consolev1.ClusterType_CLUSTER_TYPE_INVALID
	}
	return v
}

func HostTypeToGRPC(hostType hosts.Role) consolev1.HostType {
	switch hostType {
	case hosts.RoleClickHouse:
		return consolev1.HostType_HOST_TYPE_CLICKHOUSE
	case hosts.RoleKafka:
		return consolev1.HostType_HOST_TYPE_KAFKA
	default:
		return consolev1.HostType_HOST_TYPE_INVALID
	}
}

func ClusterResourcesToGRPC(clusterResources consolemodels.ClusterResources) *consolev1.Cluster_Resources {
	return &consolev1.Cluster_Resources{
		HostType:         HostTypeToGRPC(clusterResources.HostType),
		HostCount:        clusterResources.HostCount,
		ResourcePresetId: clusterResources.ResourcePresetExtID,
		DiskSize:         clusterResources.DiskSize,
	}
}

func AllClusterResourcesToGRPC(clusterType clusters.Type, allClusterResources []consolemodels.ClusterResources) []*consolev1.Cluster_Resources {
	var res []*consolev1.Cluster_Resources

	for _, clusterResources := range allClusterResources {
		// For kafka in data cloud we shouldn't show ZK nodes
		shouldSkip := clusterType == clusters.TypeKafka && clusterResources.HostType == hosts.RoleZooKeeper
		if !shouldSkip {
			res = append(res, ClusterResourcesToGRPC(clusterResources))
		}
	}

	return res
}

func operationMetadataAsMap(op opmodels.Operation) (map[string]string, error) {
	md, ok := op.MetaData.(opmodels.Metadata)
	if !ok {
		return nil, xerrors.Errorf("missing metadata in operation %q", op.OperationID)
	}

	marshaller := jsonpb.Marshaler{
		OrigName:     true,
		EnumsAsInts:  false,
		EmitDefaults: true,
		Indent:       "",
		AnyResolver:  nil,
	}
	mdJSON, err := marshaller.MarshalToString(md.Build(op))
	if err != nil {
		return nil, err
	}

	res := map[string]string{}
	err = json.Unmarshal([]byte(mdJSON), &res)
	if err != nil {
		return nil, err
	}

	return res, nil
}

func operationStatusToGRPC(opStatus opmodels.Status) apiv1.Operation_Status {
	switch opStatus {
	default:
		return apiv1.Operation_STATUS_INVALID
	case opmodels.StatusPending:
		return apiv1.Operation_STATUS_PENDING
	case opmodels.StatusRunning:
		return apiv1.Operation_STATUS_RUNNING
	case opmodels.StatusDone:
		return apiv1.Operation_STATUS_DONE
	}
}

func OperationToGRPC(ctx context.Context, op opmodels.Operation, projectID string, logger log.Logger) (*apiv1.Operation, error) {
	opGrpc, err := grpc.OperationToGRPC(ctx, op, logger)
	if err != nil {
		return nil, err
	}

	opMetadata, err := operationMetadataAsMap(op)
	if err != nil {
		return nil, err
	}

	res := apiv1.Operation{
		Id:          opGrpc.GetId(),
		ProjectId:   projectID,
		Description: opGrpc.GetDescription(),
		CreatedBy:   opGrpc.CreatedBy,
		Metadata:    opMetadata,
		CreateTime:  grpc.TimeToGRPC(op.CreatedAt),
		StartTime:   grpc.TimeToGRPC(op.StartedAt),
		FinishTime:  grpc.TimeToGRPC(op.ModifiedAt),
		Status:      operationStatusToGRPC(op.Status),
		Error:       opGrpc.GetError(),
	}

	return &res, nil
}

func OperationsToGRPC(ctx context.Context, ops []opmodels.Operation, projectID string, l log.Logger) ([]*apiv1.Operation, error) {
	var res []*apiv1.Operation

	for _, op := range ops {
		db, _ := op.Type.Database()
		if db == opmodels.DatabaseClickhouse || db == opmodels.DatabaseKafka {
			opGrpc, err := OperationToGRPC(ctx, op, projectID, l)
			if err != nil {
				return nil, err
			}
			res = append(res, opGrpc)
		}
	}

	return res, nil
}
