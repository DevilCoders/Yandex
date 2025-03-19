package airflow

import (
	"context"

	afv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/airflow/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/afmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

// ClusterService implements DB-specific gRPC methods
type ClusterService struct {
	afv1.UnimplementedClusterServiceServer

	cs  *grpcapi.ClusterService
	af  airflow.Airflow
	ops common.Operations
}

var _ afv1.ClusterServiceServer = &ClusterService{}

func NewClusterService(cs *grpcapi.ClusterService, af airflow.Airflow, ops common.Operations) *ClusterService {
	return &ClusterService{
		cs:  cs,
		af:  af,
		ops: ops,
	}
}

func (cs *ClusterService) Get(ctx context.Context, req *afv1.GetClusterRequest) (*afv1.Cluster, error) {
	cluster, err := cs.af.MDBCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return ClusterWithPillarToGRPC(cluster), nil
}

func (cs *ClusterService) Create(ctx context.Context, req *afv1.CreateClusterRequest) (*operation.Operation, error) {
	var triggerer *afmodels.TriggererConfig
	if req.GetConfig().GetTriggerer() != nil {
		triggerer = &afmodels.TriggererConfig{
			Count:     req.GetConfig().GetTriggerer().GetCount(),
			Resources: ResourcesFromGRPC(req.GetConfig().GetTriggerer().GetResources()),
		}
	}
	configSpec := afmodels.ClusterConfigSpec{
		Version: req.GetConfig().GetVersionId(),
		Airflow: afmodels.AirflowConfig{
			Config: req.GetConfig().GetAirflow().GetConfig(),
		},
		Webserver: afmodels.WebserverConfig{
			Count:     req.GetConfig().GetWebserver().GetCount(),
			Resources: ResourcesFromGRPC(req.GetConfig().GetWebserver().GetResources()),
		},
		Scheduler: afmodels.SchedulerConfig{
			Count:     req.GetConfig().GetScheduler().GetCount(),
			Resources: ResourcesFromGRPC(req.GetConfig().GetScheduler().GetResources()),
		},
		Triggerer: triggerer,
		Worker: afmodels.WorkerConfig{
			MinCount:        req.GetConfig().GetWorker().GetMinCount(),
			MaxCount:        req.GetConfig().GetWorker().GetMaxCount(),
			Resources:       ResourcesFromGRPC(req.GetConfig().GetWorker().GetResources()),
			PollingInterval: req.GetConfig().GetWorker().GetPollingInterval().AsDuration(),
			CooldownPeriod:  req.GetConfig().GetWorker().GetCooldownPeriod().AsDuration(),
		},
	}

	args := airflow.CreateMDBClusterArgs{
		FolderExtID: req.GetFolderId(),
		Name:        req.GetName(),
		Description: req.GetDescription(),
		Labels:      req.GetLabels(),
		Environment: environment.SaltEnvDev,
		ConfigSpec:  configSpec,
		Network: afmodels.NetworkConfig{
			SecurityGroupIDs: req.GetNetwork().GetSecurityGroupIds(),
			SubnetIDs:        req.GetNetwork().GetSubnetIds(),
		},
		CodeSync: afmodels.CodeSyncConfig{
			Bucket: req.GetCodeSync().GetS3Bucket(),
		},
		DeletionProtection: req.GetDeletionProtection(),
	}
	op, err := cs.af.CreateMDBCluster(ctx, args)

	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}

func (cs *ClusterService) Delete(ctx context.Context, req *afv1.DeleteClusterRequest) (*operation.Operation, error) {
	op, err := cs.af.DeleteCluster(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, cs.cs.L)
}
