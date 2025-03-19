package swagger

import (
	"time"

	swagmodels "a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func newServiceHealthFromModel(shmodel *swagmodels.ServiceHealth) types.ServiceHealth {
	return types.NewServiceHealth(
		shmodel.Name,
		time.Unix(shmodel.Timestamp, 0),
		types.ServiceStatus(*shmodel.Status),
		types.ServiceRole(shmodel.Role),
		types.ServiceReplicaType(shmodel.Replicatype),
		shmodel.ReplicaUpstream,
		shmodel.ReplicaLag,
		shmodel.Metrics,
	)
}

func newServiceHealthToModel(sh types.ServiceHealth) *swagmodels.ServiceHealth {
	status := swagmodels.ServiceStatus(sh.Status())
	return &swagmodels.ServiceHealth{
		Name:            sh.Name(),
		Timestamp:       sh.Timestamp().Unix(),
		Status:          &status,
		Role:            swagmodels.ServiceRole(sh.Role()),
		Replicatype:     swagmodels.ServiceReplicaType(sh.ReplicaType()),
		ReplicaUpstream: sh.ReplicaUpstream(),
		ReplicaLag:      sh.ReplicaLag(),
		Metrics:         sh.Metrics(),
	}
}

func newCPUMetricToModel(cpu *types.CPUMetric) *swagmodels.CPUMetrics {
	if cpu == nil {
		return nil
	}

	return &swagmodels.CPUMetrics{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func newMemoryMetricToModel(memory *types.MemoryMetric) *swagmodels.MemoryMetrics {
	if memory == nil {
		return nil
	}

	return &swagmodels.MemoryMetrics{
		Timestamp: memory.Timestamp,
		Total:     memory.Total,
		Used:      memory.Used,
	}
}

func newDiskMetricToModel(disk *types.DiskMetric) *swagmodels.DiskMetrics {
	if disk == nil {
		return nil
	}

	return &swagmodels.DiskMetrics{
		Timestamp: disk.Timestamp,
		Total:     disk.Total,
		Used:      disk.Used,
	}
}

func newSystemMetricsToModel(sm *types.SystemMetrics) *swagmodels.HostSystemMetrics {
	if sm == nil {
		return nil
	}

	return &swagmodels.HostSystemMetrics{
		CPU:  newCPUMetricToModel(sm.CPU),
		Mem:  newMemoryMetricToModel(sm.Memory),
		Disk: newDiskMetricToModel(sm.Disk),
	}
}

func newCPUMetricFromModel(cpu *swagmodels.CPUMetrics) *types.CPUMetric {
	if cpu == nil {
		return nil
	}

	return &types.CPUMetric{
		BaseMetric: types.BaseMetric{Timestamp: cpu.Timestamp},
		Used:       cpu.Used,
	}
}

func newMemoryMetricFromModel(memory *swagmodels.MemoryMetrics) *types.MemoryMetric {
	if memory == nil {
		return nil
	}

	return &types.MemoryMetric{
		BaseMetric: types.BaseMetric{Timestamp: memory.Timestamp},
		Used:       memory.Used,
		Total:      memory.Total,
	}
}

func newDiskMetricFromModel(disk *swagmodels.DiskMetrics) *types.DiskMetric {
	if disk == nil {
		return nil
	}

	return &types.DiskMetric{
		BaseMetric: types.BaseMetric{Timestamp: disk.Timestamp},
		Used:       disk.Used,
		Total:      disk.Total,
	}
}

func newSystemMetricsFromModel(smmodel *swagmodels.HostSystemMetrics) *types.SystemMetrics {
	if smmodel == nil {
		return nil
	}

	return &types.SystemMetrics{
		CPU:    newCPUMetricFromModel(smmodel.CPU),
		Memory: newMemoryMetricFromModel(smmodel.Mem),
		Disk:   newDiskMetricFromModel(smmodel.Disk),
	}
}

func newHostHealthFromModel(hhmodel *swagmodels.HostHealth) types.HostHealth {
	shs := make([]types.ServiceHealth, 0, len(hhmodel.Services))
	for _, shmodel := range hhmodel.Services {
		shs = append(shs, newServiceHealthFromModel(shmodel))
	}

	return types.NewHostHealthWithSystemAndStatus(
		hhmodel.Cid,
		hhmodel.Fqdn,
		shs,
		newSystemMetricsFromModel(hhmodel.System),
		types.HostStatus(hhmodel.Status),
	)
}

func newHostHealthToModel(hh types.HostHealth) *swagmodels.HostHealth {
	shmodels := make([]*swagmodels.ServiceHealth, 0, len(hh.Services()))
	for _, sh := range hh.Services() {
		shmodels = append(shmodels, newServiceHealthToModel(sh))
	}

	return &swagmodels.HostHealth{
		Cid:      hh.ClusterID(),
		Fqdn:     hh.FQDN(),
		Services: shmodels,
		System:   newSystemMetricsToModel(hh.System()),
	}
}

func newClusterHealthFromModel(smod *swagmodels.ClusterHealth) (client.ClusterHealth, error) {
	var s client.ClusterStatus
	switch *smod.Status {
	case swagmodels.ClusterStatusAlive:
		s = client.ClusterStatusAlive
	case swagmodels.ClusterStatusDead:
		s = client.ClusterStatusDead
	case swagmodels.ClusterStatusDegraded:
		s = client.ClusterStatusDegraded
	case swagmodels.ClusterStatusUnknown:
		s = client.ClusterStatusUnknown

	default:
		return client.ClusterHealth{}, xerrors.Errorf("unexpected cluster status: %q", *smod.Status)
	}
	return client.ClusterHealth{
		Status:             s,
		Timestamp:          time.Unix(smod.Timestamp, 0),
		LastAliveTimestamp: time.Unix(smod.Lastalivetimestamp, 0),
	}, nil
}
