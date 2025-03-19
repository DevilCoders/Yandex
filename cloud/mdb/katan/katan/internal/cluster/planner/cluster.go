package planner

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	healthtypes "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

// Service is service that run on host
type Service struct {
	Role        healthtypes.ServiceRole
	ReplicaType healthtypes.ServiceReplicaType
	Status      healthtypes.ServiceStatus
}

type Host struct {
	// Services by its name
	Services map[string]Service
	Tags     tags.HostTags
}

// Structure for
type Cluster struct {
	ID    string
	Tags  tags.ClusterTags
	Hosts map[string]Host
}

// ComposeCluster takes models.Cluster, mdb-health response and compose Cluster
func ComposeCluster(cluster models.Cluster, hostsHealth []healthtypes.HostHealth) (Cluster, error) {
	ret := Cluster{
		ID:    cluster.ID,
		Tags:  cluster.Tags,
		Hosts: make(map[string]Host, len(cluster.Hosts)),
	}

	for fqdn, t := range cluster.Hosts {
		ret.Hosts[fqdn] = Host{Tags: t}
	}

	for _, hh := range hostsHealth {
		services := make(map[string]Service, len(hh.Services()))
		for _, s := range hh.Services() {
			if _, ok := services[s.Name()]; ok {
				return Cluster{}, fmt.Errorf("duplicate service %q. host health: %+v", s.Name(), hh)
			}
			services[s.Name()] = Service{
				Role:        s.Role(),
				ReplicaType: s.ReplicaType(),
				Status:      s.Status(),
			}
		}
		host, ok := ret.Hosts[hh.FQDN()]
		if !ok {
			return Cluster{}, fmt.Errorf("%q host not exists in cluster: %+v, but present in health: %+v", hh.FQDN(), cluster, hh)
		}
		host.Services = services
		ret.Hosts[hh.FQDN()] = host
	}
	return ret, nil
}
