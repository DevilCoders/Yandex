package testhelpers

import (
	"math/rand"
	"time"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

// NewBaseMetric generates system metrics
func NewBaseMetric() types.BaseMetric {
	return types.BaseMetric{Timestamp: time.Now().Unix()}
}

func randomUsedAndTotal() (int64, int64) {
	used := rand.Uint32()
	total := rand.Uint32()
	if used > total {
		used, total = total, used
	}

	return int64(used), int64(total)
}

// NewCPUMetric generates cpu metrics
func NewCPUMetric() *types.CPUMetric {
	return &types.CPUMetric{
		BaseMetric: NewBaseMetric(),
		Used:       rand.Float64(),
	}
}

// NewMemoryMetric generates cpu metrics
func NewMemoryMetric() *types.MemoryMetric {
	used, total := randomUsedAndTotal()
	return &types.MemoryMetric{
		BaseMetric: NewBaseMetric(),
		Used:       used,
		Total:      total,
	}
}

// NewDiskMetric generates cpu metrics
func NewDiskMetric() *types.DiskMetric {
	used, total := randomUsedAndTotal()
	return &types.DiskMetric{
		BaseMetric: NewBaseMetric(),
		Used:       used,
		Total:      total,
	}
}

// NewSystemMetrics generates system metrics
func NewSystemMetrics() *types.SystemMetrics {
	return &types.SystemMetrics{
		CPU:    NewCPUMetric(),
		Memory: NewMemoryMetric(),
		Disk:   NewDiskMetric(),
	}
}

// NewMetrics generates metricsCount amount of metrics data
func NewMetrics(metricsCount int) map[string]string {
	metrics := make(map[string]string)
	for i := 0; i < metricsCount; i++ {
		metrics[uuid.Must(uuid.NewV4()).String()] = uuid.Must(uuid.NewV4()).String()
	}

	return metrics
}

// NewMasterServiceHealth generates master service health data with metricsCount amount of metrics data
func NewMasterServiceHealth(metricsCount int) types.ServiceHealth {
	return types.NewServiceHealth(
		uuid.Must(uuid.NewV4()).String(),
		time.Now(),
		types.ServiceStatusAlive,
		types.ServiceRoleMaster,
		types.ServiceReplicaTypeUnknown,
		"",
		0,
		NewMetrics(metricsCount),
	)
}

func newReplicaServiceHealth(metricsCount int, replicaType types.ServiceReplicaType, replicaUpstream string, replicaLag int64) types.ServiceHealth {
	return types.NewServiceHealth(
		uuid.Must(uuid.NewV4()).String(),
		time.Now(),
		types.ServiceStatusAlive,
		types.ServiceRoleReplica,
		replicaType,
		replicaUpstream,
		replicaLag,
		NewMetrics(metricsCount),
	)
}

func newSyncReplicaServiceHealth(metricsCount int) types.ServiceHealth {
	return newReplicaServiceHealth(metricsCount, types.ServiceReplicaTypeSync, "", 0)
}

func newAsyncReplicaServiceHealth(metricsCount int) types.ServiceHealth {
	return newReplicaServiceHealth(metricsCount, types.ServiceReplicaTypeAsync, "", 0)
}

func newQuorumReplicaServiceHealth(metricsCount int) types.ServiceHealth {
	return newReplicaServiceHealth(metricsCount, types.ServiceReplicaTypeQuorum, "", 0)
}

func NewQuorumReplicaHealthWithUpstreamAndLag(metricsCount int, replicaUpstream string, replicaLag int64) types.ServiceHealth {
	return newReplicaServiceHealth(metricsCount, types.ServiceReplicaTypeQuorum, replicaUpstream, replicaLag)
}

func newUnknownServiceHealth(metricsCount int) types.ServiceHealth {
	return types.NewUnknownServiceHealth()
}

// NewUnknownServiceHealthFromExisting generates service health data from existing data but
// sets its status and role to unknown
func NewUnknownServiceHealthFromExisting(existing types.ServiceHealth) types.ServiceHealth {
	return types.NewServiceHealth(
		existing.Name(),
		time.Time{},
		types.ServiceStatusUnknown,
		types.ServiceRoleUnknown,
		types.ServiceReplicaTypeUnknown,
		"",
		0,
		map[string]string{},
	)
}

type newServiceFunc func(metricsCoung int) types.ServiceHealth

func newServiceHealthArray(servicesCount, metricsCount int, servFunc newServiceFunc) []types.ServiceHealth {
	shs := make([]types.ServiceHealth, 0, servicesCount)
	for i := 0; i < servicesCount; i++ {
		shs = append(shs, servFunc(metricsCount))
	}

	return shs
}

// NewUnknownServiceHealthArrayFromExisting generates array of service health data from existing data but sets its
// status to unknown
func NewUnknownServiceHealthArrayFromExisting(existing []types.ServiceHealth) []types.ServiceHealth {
	shs := make([]types.ServiceHealth, 0, len(existing))
	for _, sh := range existing {
		shs = append(shs, NewUnknownServiceHealthFromExisting(sh))
	}

	return shs
}

// NewHostHealth generates host health data with servicesCount amount of service health data with metricsCount amount
// of metrics data
func NewHostHealth(servicesCount, metricsCount int) types.HostHealth {
	return hhSpecWithCid(uuid.Must(uuid.NewV4()).String(), servicesCount, metricsCount, 1)
}

// NewHostHealthWithOtherServices create new host health but with other services
func NewHostHealthWithOtherServices(hh types.HostHealth, s []types.ServiceHealth) types.HostHealth {
	return types.NewHostHealthWithSystemAndMode(hh.ClusterID(), hh.FQDN(), s, hh.System(), hh.Mode())
}

// NewHostHealthWithOtherServicesAndSystem create new host health but with other services and system metrics
func NewHostHealthWithOtherServicesAndSystem(hh types.HostHealth, s []types.ServiceHealth, sm *types.SystemMetrics) types.HostHealth {
	return types.NewHostHealthWithSystemAndMode(hh.ClusterID(), hh.FQDN(), s, sm, hh.Mode())
}

// NewHostHealthWithOtherSystem create new host health but with other system metrics
func NewHostHealthWithOtherSystem(hh types.HostHealth, sm *types.SystemMetrics) types.HostHealth {
	return types.NewHostHealthWithSystemAndMode(hh.ClusterID(), hh.FQDN(), hh.Services(), sm, hh.Mode())
}

func hhSpecWithCid(cid string, servicesCount, metricsCount, typeService int) types.HostHealth {
	servFunc := newUnknownServiceHealth
	switch typeService {
	case 1:
		servFunc = NewMasterServiceHealth
	case 2:
		servFunc = newSyncReplicaServiceHealth
	case 3:
		servFunc = newAsyncReplicaServiceHealth
	case 4:
		servFunc = newQuorumReplicaServiceHealth
	}

	sha := newServiceHealthArray(servicesCount, metricsCount, servFunc)
	sm := NewSystemMetrics()
	return types.NewHostHealthWithSystem(cid, uuid.Must(uuid.NewV4()).String(), sha, sm)
}

// NewHostHealthWithClusterID generates host health data with servicesCount amount of service health data with
// metricsCount amount of metrics data. Uses specified cluster ID.
func NewHostHealthWithClusterID(cid string, servicesCount, metricsCount int) types.HostHealth {
	return hhSpecWithCid(cid, servicesCount, metricsCount, 1)
}

// NewHostHealthSpec generates host health data with servicesCount amount of service health data with
// metricsCount amount of metrics data with special type service
func NewHostHealthSpec(servicesCount, metricsCount, typeService int) types.HostHealth {
	return hhSpecWithCid(uuid.Must(uuid.NewV4()).String(), servicesCount, metricsCount, typeService)
}

// NewUnknownHostHealthFromExisting generates host health data from existing data but sets its status to unknown
func NewUnknownHostHealthFromExisting(existing types.HostHealth) types.HostHealth {
	return types.NewHostHealthWithSystemAndMode(
		existing.ClusterID(),
		existing.FQDN(),
		NewUnknownServiceHealthArrayFromExisting(existing.Services()),
		NewSystemMetrics(),
		existing.Mode(),
	)
}

// NewHostHealthArray generates hostsCount amount of host health data with servicesCount amount of service health data
// with metricsCount amount of metrics data.
func NewHostHealthArray(hostsCount, servicesCount, metricsCount int) []types.HostHealth {
	return NewHostHealthArrayWithClusterID(uuid.Must(uuid.NewV4()).String(), hostsCount, servicesCount, metricsCount)
}

// NewHostHealthArrayWithClusterID generates hostsCount amount of host health data with servicesCount amount of service
// health data with metricsCount amount of metrics data. Uses specified cluster ID.
func NewHostHealthArrayWithClusterID(cid string, hostsCount, servicesCount, metricsCount int) []types.HostHealth {
	hhs := make([]types.HostHealth, 0, hostsCount)
	for i := 0; i < hostsCount; i++ {
		hhs = append(hhs, NewHostHealthWithClusterID(cid, servicesCount, metricsCount))
	}

	return hhs
}

// NewUnknownHostHealthArrayFromExisting generates array of host health data from existing data but sets its status
// to unknown
func NewUnknownHostHealthArrayFromExisting(existing []types.HostHealth) []types.HostHealth {
	hhs := make([]types.HostHealth, 0, len(existing))
	for _, hh := range existing {
		hhs = append(hhs, NewUnknownHostHealthFromExisting(hh))
	}

	return hhs
}
