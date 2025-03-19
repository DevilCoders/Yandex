package models

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
)

// ClusterTopology describes cluster topology - subclusters, hosts, and services
type ClusterTopology struct {
	Cid              string
	Revision         int64
	FolderID         string
	Services         []service.Service
	Subclusters      []SubclusterTopology
	UIProxy          bool
	ServiceAccountID string
}

// SubclusterTopology describes subcluster
type SubclusterTopology struct {
	Subcid              string
	Role                role.Role
	Services            []service.Service
	Hosts               []string
	MinHostsCount       int64 // for instance group based subclusters
	InstanceGroupID     string
	DecommissionTimeout int64
}

// HasService checks if subcluster provides service
func (st *SubclusterTopology) HasService(serviceType service.Service) bool {
	for _, subService := range st.Services {
		if subService == serviceType {
			return true
		}
	}
	return false
}

// MarshalBinary provides binary marshaling for cluster topology
func (tp *ClusterTopology) MarshalBinary() ([]byte, error) {
	return json.Marshal(tp)
}

// UnmarshalBinary provides unmarshaling for cluster topology
func (tp *ClusterTopology) UnmarshalBinary(data []byte) error {
	return json.Unmarshal(data, tp)
}

// HasService checks if cluster provides service
func (tp *ClusterTopology) HasService(serviceType service.Service) bool {
	for _, topologyService := range tp.Services {
		if serviceType == topologyService {
			return true
		}
	}
	return false
}

// GetInstanceGroupIDs returns IDs of instance groups associated with the cluster
func (tp *ClusterTopology) GetInstanceGroupIDs() []string {
	var instanceGroupIDs []string
	for _, subcluster := range tp.Subclusters {
		if subcluster.InstanceGroupID != "" {
			instanceGroupIDs = append(instanceGroupIDs, subcluster.InstanceGroupID)
		}
	}
	return instanceGroupIDs
}

// GetSubclusterByInstanceGroupID returns subcluster topology by instance group ID
func (tp *ClusterTopology) GetSubclusterByInstanceGroupID(instanceGroupID string) *SubclusterTopology {
	for _, subcluster := range tp.Subclusters {
		if subcluster.InstanceGroupID == instanceGroupID {
			return &subcluster
		}
	}
	return nil
}

// ServiceNodes returns all fqdns that provides service
func (tp *ClusterTopology) ServiceNodes(serviceType service.Service) []string {
	fqdns := make([]string, 0)
	for _, subcluster := range tp.Subclusters {
		if !subcluster.HasService(serviceType) {
			continue
		}
		fqdns = append(fqdns, subcluster.Hosts...)
	}
	return fqdns
}

// AllNodes returns all fqdns from all subclusters
func (tp *ClusterTopology) AllNodes() []string {
	fqdns := make([]string, 0)
	for _, subcluster := range tp.Subclusters {
		if subcluster.Role == role.Main {
			continue
		}
		fqdns = append(fqdns, subcluster.Hosts...)
	}
	return fqdns
}

// MasterNodes returns all fqdns from main subclusters
func (tp *ClusterTopology) MasterNodes() []string {
	fqdns := make([]string, 0)
	for _, subcluster := range tp.Subclusters {
		if subcluster.Role != role.Main {
			continue
		}
		fqdns = append(fqdns, subcluster.Hosts...)
	}
	return fqdns
}
