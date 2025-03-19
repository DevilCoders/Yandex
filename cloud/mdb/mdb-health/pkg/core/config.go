package core

import "time"

// GWConfig GeneralWard config
type GWConfig struct {
	SecretTimeout        time.Duration `json:"secret_timeout" yaml:"secret_timeout"`
	HostHealthTimeout    time.Duration `json:"host_health_timeout" yaml:"host_health_timeout"`
	ClusterHealthTimeout time.Duration `json:"cluster_health_timeout" yaml:"cluster_health_timeout"`
	LeadTimeout          time.Duration `json:"lead_timeout" yaml:"lead_timeout"`
	TopologyTimeout      time.Duration `json:"topology_timeout" yaml:"topology_timeout"`
	UpdTopologyTimeout   time.Duration `json:"update_topology_timeout" yaml:"update_topology_timeout"`
}

// DefaultConfig return base logic default config
func DefaultConfig() GWConfig {
	return GWConfig{
		SecretTimeout:        24 * time.Hour,
		HostHealthTimeout:    30 * time.Second, // big enough because mdb_metrics sends to Solomon 15 seconds slots
		ClusterHealthTimeout: 24 * time.Hour,
		LeadTimeout:          10 * time.Second,
		TopologyTimeout:      4 * 24 * time.Hour,
		UpdTopologyTimeout:   2 * time.Minute,
	}
}
