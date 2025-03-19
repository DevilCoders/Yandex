package models

import (
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type HostHealth struct {
	ClusterID string    `db:"cluster_id"`
	FQDN      string    `db:"fqdn"`
	Created   time.Time `db:"created"`
	Status    string    `db:"status"`
	Expired   time.Time `db:"expired"`

	ServicesName            []string    `db:"services.name"`
	ServicesServiceTime     []time.Time `db:"services.service_time"`
	ServicesStatus          []string    `db:"services.status"`
	ServicesRole            []string    `db:"services.role"`
	ServicesReplicaType     []string    `db:"services.replica_type"`
	ServicesReplicaUpstream []string    `db:"services.replica_upstream"`
	ServicesReplicaLag      []int64     `db:"services.replica_lag"`
	ServicesMetrics         [][]byte    `db:"services.metrics"`

	SystemMetricsMetricType []string    `db:"system_metrics.metric_type"`
	SystemMetricsMetricTime []time.Time `db:"system_metrics.metric_time"`
	SystemMetricsUsed       []float64   `db:"system_metrics.used"`
	SystemMetricsTotal      []float64   `db:"system_metrics.total"`

	Mode          []uint8   `db:"mode"`
	ModeTimestamp time.Time `db:"mode_timestamp"`
}

func (h *HostHealth) ToInsert() []interface{} {
	args := []interface{}{
		h.ClusterID,
		h.FQDN,
		h.Created,
		h.Status,
		h.Expired,
		h.ServicesName,
		h.ServicesServiceTime,
		h.ServicesStatus,
		h.ServicesRole,
		h.ServicesReplicaType,
		h.ServicesReplicaUpstream,
		h.ServicesReplicaLag,
		h.ServicesMetrics,
		h.SystemMetricsMetricType,
		h.SystemMetricsMetricTime,
		h.SystemMetricsUsed,
		h.SystemMetricsTotal,
		h.Mode,
		h.ModeTimestamp,
	}
	return args
}

func HostHealthToInternal(hh healthstore.HostHealthToStore) (HostHealth, error) {
	now := time.Now()
	res := HostHealth{
		ClusterID: hh.Health.ClusterID(),
		FQDN:      hh.Health.FQDN(),
		Created:   now,
		Status:    string(hh.Health.Status()),
		Expired:   now.Add(hh.TTL),
	}
	if err := ServiceToInternal(hh.Health.Services(), &res); err != nil {
		return res, xerrors.Errorf("convert services to internal model for fqdn %q: %w", res.FQDN, err)
	}
	SystemMetricsToInternal(hh.Health.System(), &res)
	ModeToInternal(hh.Health.Mode(), &res)

	return res, nil
}

func ServiceToInternal(services []types.ServiceHealth, hh *HostHealth) error {
	hh.ServicesName = make([]string, len(services))
	hh.ServicesServiceTime = make([]time.Time, len(services))
	hh.ServicesStatus = make([]string, len(services))
	hh.ServicesRole = make([]string, len(services))
	hh.ServicesReplicaType = make([]string, len(services))
	hh.ServicesReplicaUpstream = make([]string, len(services))
	hh.ServicesReplicaLag = make([]int64, len(services))
	hh.ServicesMetrics = make([][]byte, len(services))

	for i, service := range services {
		hh.ServicesName[i] = service.Name()
		hh.ServicesServiceTime[i] = service.Timestamp()
		hh.ServicesStatus[i] = string(service.Status())
		hh.ServicesRole[i] = string(service.Role())
		hh.ServicesReplicaType[i] = string(service.ReplicaType())
		hh.ServicesReplicaUpstream[i] = service.ReplicaUpstream()
		hh.ServicesReplicaLag[i] = service.ReplicaLag()

		data, err := json.Marshal(service.Metrics())
		if err != nil {
			return xerrors.Errorf("json marshal service name %q: %w", service.Name(), err)
		}
		hh.ServicesMetrics[i] = data
	}
	return nil
}

func SystemMetricsToInternal(metrics *types.SystemMetrics, hh *HostHealth) {
	if metrics == nil {
		return
	}
	hh.SystemMetricsMetricType = make([]string, 0, 3)
	hh.SystemMetricsMetricTime = make([]time.Time, 0, 3)
	hh.SystemMetricsUsed = make([]float64, 0, 3)
	hh.SystemMetricsTotal = make([]float64, 0, 3)

	if metrics.CPU != nil {
		hh.SystemMetricsMetricType = append(hh.SystemMetricsMetricType, "cpu")
		hh.SystemMetricsMetricTime = append(hh.SystemMetricsMetricTime, time.Unix(metrics.CPU.Timestamp, 0))
		hh.SystemMetricsUsed = append(hh.SystemMetricsUsed, metrics.CPU.Used)
		hh.SystemMetricsTotal = append(hh.SystemMetricsTotal, 0)
	}

	if metrics.Memory != nil {
		hh.SystemMetricsMetricType = append(hh.SystemMetricsMetricType, "memory")
		hh.SystemMetricsMetricTime = append(hh.SystemMetricsMetricTime, time.Unix(metrics.Memory.Timestamp, 0))
		hh.SystemMetricsUsed = append(hh.SystemMetricsUsed, float64(metrics.Memory.Used))
		hh.SystemMetricsTotal = append(hh.SystemMetricsTotal, float64(metrics.Memory.Total))
	}

	if metrics.Disk != nil {
		hh.SystemMetricsMetricType = append(hh.SystemMetricsMetricType, "disk")
		hh.SystemMetricsMetricTime = append(hh.SystemMetricsMetricTime, time.Unix(metrics.Disk.Timestamp, 0))
		hh.SystemMetricsUsed = append(hh.SystemMetricsUsed, float64(metrics.Disk.Used))
		hh.SystemMetricsTotal = append(hh.SystemMetricsTotal, float64(metrics.Disk.Total))
	}
}

func ModeToInternal(mode *types.Mode, hh *HostHealth) {
	if mode == nil {
		return
	}
	hh.ModeTimestamp = mode.Timestamp
	hh.Mode = make([]uint8, 3)
	if mode.Read {
		hh.Mode[0] = 1
	}
	if mode.Write {
		hh.Mode[1] = 1
	}
	if mode.UserFaultBroken {
		hh.Mode[2] = 1
	}
}
