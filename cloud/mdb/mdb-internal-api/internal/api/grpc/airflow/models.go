package airflow

import (
	"google.golang.org/protobuf/types/known/durationpb"

	afv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/airflow/v1"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/afmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

func ClusterWithPillarToGRPC(cluster afmodels.MDBCluster) *afv1.Cluster {
	v := ClusterToGRPC(cluster)
	v.Config = &afv1.ClusterConfig{
		VersionId: cluster.Config.Version,
		Airflow: &afv1.AirflowConfig{
			Config: cluster.Config.Airflow.Config,
		},
		Webserver: &afv1.WebserverConfig{
			Count:     cluster.Config.Webserver.Count,
			Resources: ResourcesToGRPC(&cluster.Config.Webserver.Resources),
		},
		Scheduler: &afv1.SchedulerConfig{
			Count:     cluster.Config.Scheduler.Count,
			Resources: ResourcesToGRPC(&cluster.Config.Scheduler.Resources),
		},
		Worker: &afv1.WorkerConfig{
			// TODO: Propagate min/max count
			MinCount:        cluster.Config.Worker.MinCount,
			MaxCount:        cluster.Config.Worker.MaxCount,
			Resources:       ResourcesToGRPC(&cluster.Config.Worker.Resources),
			PollingInterval: durationpb.New(cluster.Config.Worker.PollingInterval),
			CooldownPeriod:  durationpb.New(cluster.Config.Worker.CooldownPeriod),
		},
	}
	if cluster.Config.Triggerer != nil {
		v.Config.Triggerer = &afv1.TriggererConfig{
			Count:     cluster.Config.Triggerer.Count,
			Resources: ResourcesToGRPC(&cluster.Config.Triggerer.Resources),
		}
	}
	v.Monitoring = make([]*afv1.Monitoring, 0, len(cluster.Monitoring.Charts))
	for _, chart := range cluster.Monitoring.Charts {
		mon := &afv1.Monitoring{
			Name:        chart.Name,
			Description: chart.Description,
			Link:        chart.Link,
		}
		v.Monitoring = append(v.Monitoring, mon)
	}
	return v
}

func ClusterToGRPC(cluster afmodels.MDBCluster) *afv1.Cluster {
	v := &afv1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		CreatedAt:          grpcapi.TimeToGRPC(cluster.CreatedAt),
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		Environment:        afv1.Cluster_PRESTABLE,
		Status:             StatusToGRPC(cluster.Status),
		Health:             ClusterHealthToGRPC(cluster.Health),
		DeletionProtection: cluster.DeletionProtection,

		Network: &afv1.NetworkConfig{
			SubnetIds:        cluster.SubnetIDs,
			SecurityGroupIds: cluster.SecurityGroupIDs,
		},
		CodeSync: &afv1.CodeSyncConfig{
			S3Bucket: cluster.CodeSync.Bucket,
		},
	}
	return v
}

var (
	mapStatusToGRCP = map[clusters.Status]afv1.Cluster_Status{
		clusters.StatusCreating:             afv1.Cluster_CREATING,
		clusters.StatusCreateError:          afv1.Cluster_ERROR,
		clusters.StatusRunning:              afv1.Cluster_RUNNING,
		clusters.StatusModifyError:          afv1.Cluster_ERROR,
		clusters.StatusStopping:             afv1.Cluster_STOPPING,
		clusters.StatusStopped:              afv1.Cluster_STOPPED,
		clusters.StatusStopError:            afv1.Cluster_ERROR,
		clusters.StatusStarting:             afv1.Cluster_STARTING,
		clusters.StatusStartError:           afv1.Cluster_ERROR,
		clusters.StatusMaintainOfflineError: afv1.Cluster_ERROR,
	}
)

func StatusToGRPC(status clusters.Status) afv1.Cluster_Status {
	v, ok := mapStatusToGRCP[status]
	if !ok {
		return afv1.Cluster_STATUS_UNKNOWN
	}
	return v
}

var (
	mapClusterHealthToGRPC = map[clusters.Health]afv1.Health{
		clusters.HealthAlive:    afv1.Health_ALIVE,
		clusters.HealthDegraded: afv1.Health_DEGRADED,
		clusters.HealthDead:     afv1.Health_DEAD,
		clusters.HealthUnknown:  afv1.Health_HEALTH_UNKNOWN,
	}
)

func ClusterHealthToGRPC(env clusters.Health) afv1.Health {
	v, ok := mapClusterHealthToGRPC[env]
	if !ok {
		return afv1.Health_HEALTH_UNKNOWN
	}
	return v
}

func ResourcesFromGRPC(res *afv1.Resources) afmodels.Resources {
	return afmodels.Resources{
		VCPUCount:   res.GetVcpuCount(),
		MemoryBytes: res.GetMemoryBytes(),
	}
}

func ResourcesToGRPC(res *afmodels.Resources) *afv1.Resources {
	return &afv1.Resources{
		VcpuCount:   res.VCPUCount,
		MemoryBytes: res.MemoryBytes,
	}
}
