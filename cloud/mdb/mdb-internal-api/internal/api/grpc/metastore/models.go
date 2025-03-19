package metastore

import (
	msv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/metastore/v1"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

var (
	mapStatusToGRCP = map[clusters.Status]msv1.Cluster_Status{
		clusters.StatusCreating:    msv1.Cluster_CREATING,
		clusters.StatusCreateError: msv1.Cluster_ERROR,
		clusters.StatusRunning:     msv1.Cluster_RUNNING,
		clusters.StatusModifying:   msv1.Cluster_UPDATING,
		clusters.StatusModifyError: msv1.Cluster_ERROR,
		clusters.StatusStopping:    msv1.Cluster_STOPPING,
		clusters.StatusStopped:     msv1.Cluster_STOPPED,
		clusters.StatusStopError:   msv1.Cluster_ERROR,
		clusters.StatusStarting:    msv1.Cluster_STARTING,
		clusters.StatusStartError:  msv1.Cluster_ERROR,
	}
)

func StatusToGRPC(status clusters.Status) msv1.Cluster_Status {
	v, ok := mapStatusToGRCP[status]
	if !ok {
		return msv1.Cluster_STATUS_UNKNOWN
	}
	return v
}

func ClusterToGRPC(cluster models.MDBCluster) *msv1.Cluster {
	v := &msv1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		CreatedAt:          grpcapi.TimeToGRPC(cluster.CreatedAt),
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		SecurityGroupIds:   cluster.SecurityGroupIDs,
		HostGroupIds:       cluster.HostGroupIDs,
		Status:             StatusToGRPC(cluster.Status),
		Health:             ClusterHealthToGRPC(cluster.Health),
		DeletionProtection: cluster.DeletionProtection,
		MinServersPerZone:  cluster.Config.MinServersPerZone,
		MaxServersPerZone:  cluster.Config.MaxServersPerZone,
		Version:            cluster.Config.Version,
		SubnetIds:          cluster.Config.SubnetIDs,
		NetworkId:          cluster.Config.NetworkID,
		EndpointIp:         cluster.Config.EndpointIP,
	}
	return v
}

var (
	mapClusterHealthToGRPC = map[clusters.Health]msv1.Cluster_Health{
		clusters.HealthAlive:    msv1.Cluster_ALIVE,
		clusters.HealthDegraded: msv1.Cluster_DEGRADED,
		clusters.HealthDead:     msv1.Cluster_DEAD,
		clusters.HealthUnknown:  msv1.Cluster_HEALTH_UNKNOWN,
	}
)

func ClusterHealthToGRPC(env clusters.Health) msv1.Cluster_Health {
	v, ok := mapClusterHealthToGRPC[env]
	if !ok {
		return msv1.Cluster_HEALTH_UNKNOWN
	}
	return v
}
