package models

import (
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
)

// Cluster is a cluster (group of hosts) definition in katan terms
type Cluster struct {
	ID    string
	Tags  tags.ClusterTags
	Hosts map[string]tags.HostTags
}

// FQDNs return all hosts FQDNs
func (c Cluster) FQDNs() []string {
	ret := make([]string, 0, len(c.Hosts))
	for f := range c.Hosts {
		ret = append(ret, f)
	}
	return ret
}

type ClusterChange struct {
	ClusterID string
	State     katandb.ClusterRolloutState
	Comment   string
}

type Shipment struct {
	ClusterID  string
	FQDNs      []string
	ShipmentID int64
}
