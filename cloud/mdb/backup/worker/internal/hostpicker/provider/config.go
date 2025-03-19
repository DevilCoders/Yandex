package hostpicker

import (
	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type HostPickerType string

const (
	HealthyHostPickerType       = HostPickerType("healthy")
	PreferReplicaHostPickerType = HostPickerType("prefer_replica")
)

type HostPickerConfig struct {
	HealthMaxRetries         uint64             `json:"health_max_retries" yaml:"health_max_retries"`
	HostHealthStatusesOrder  []types.HostStatus `json:"host_health_statuses_order" yaml:"host_health_statuses_order"`
	ReplicationHealthService string             `json:"replication_health_service" yaml:"replication_health_service"`
	PriorityArgs             []string           `json:"priority_args" yaml:"priority_args"`
}

type Config struct {
	HostPickerType HostPickerType   `json:"host_picker_type" yaml:"host_picker_type"`
	Config         HostPickerConfig `json:"config" yaml:"config"`
}

type HostPickersMapConfig struct {
	ClusterTypeSettings map[metadb.ClusterType]Config `json:"cluster_type_settings" yaml:"cluster_type_settings"`

	DefaultSettings Config `json:"default_settings" yaml:"default_settings"`
}

func DefaultConfig() HostPickersMapConfig {
	return HostPickersMapConfig{
		ClusterTypeSettings: map[metadb.ClusterType]Config{
			metadb.MysqlCluster: {
				HostPickerType: PreferReplicaHostPickerType,
				Config: HostPickerConfig{
					HealthMaxRetries:         3,
					HostHealthStatusesOrder:  []types.HostStatus{types.HostStatusAlive, types.HostStatusDegraded, types.HostStatusUnknown},
					ReplicationHealthService: "service_health",
					PriorityArgs:             []string{"data", "mysql", "backup_priority"},
				},
			},
		},
		DefaultSettings: Config{
			HostPickerType: HealthyHostPickerType,
			Config: HostPickerConfig{
				HealthMaxRetries:        3,
				HostHealthStatusesOrder: []types.HostStatus{types.HostStatusAlive, types.HostStatusDegraded, types.HostStatusUnknown},
			},
		},
	}
}
