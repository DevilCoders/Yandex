package types

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
)

// AggType aggregation type
type AggType string

// AggType aggregation type values
const (
	AggClusters AggType = "clusters"
	AggShards   AggType = "shards"
	AggGeoHosts AggType = "geohosts"
)

type Mode struct {
	Timestamp       time.Time
	Read            bool
	Write           bool
	UserFaultBroken bool
}

type BaseMetric struct {
	Timestamp int64
}

type CPUMetric struct {
	BaseMetric
	Used float64
}

type MemoryMetric struct {
	BaseMetric
	Used  int64
	Total int64
}

type DiskMetric struct {
	BaseMetric
	Used  int64
	Total int64
}

type SystemMetrics struct {
	CPU    *CPUMetric
	Memory *MemoryMetric
	Disk   *DiskMetric
}

// HostHealth defines status of all services of one host of cluster
type HostHealth struct {
	cid      string
	fqdn     string
	status   HostStatus
	services []ServiceHealth
	system   *SystemMetrics
	mode     *Mode
}

// HostNeighboursInfo information about ready for maintenance host
type HostNeighboursInfo struct {
	Cid            string
	Sid            string
	Env            string
	Roles          []string
	HACluster      bool
	HAShard        bool
	SameRolesTotal int
	SameRolesAlive int
	SameRolesTS    time.Time
}

func (ni *HostNeighboursInfo) IsHA() bool {
	if ni.SameRolesTotal == 0 {
		return false // one legged
	}

	return ni.HAShard || ni.HACluster
}

// NewHostHealthWithStatus constructs host health object
func NewHostHealthWithStatus(cid, fqdn string, services []ServiceHealth, status HostStatus) HostHealth {
	return NewHostHealthWithStatusAndMode(cid, fqdn, services, nil, status)
}

// NewHostHealthWithStatusAndMode constructs host health object
func NewHostHealthWithStatusAndMode(cid, fqdn string, services []ServiceHealth, mode *Mode, status HostStatus) HostHealth {
	return NewHostHealthWithStatusSystemAndMode(cid, fqdn, services, nil, mode, status)
}

// NewHostHealthWithStatusSystemAndMode constructs host health object
func NewHostHealthWithStatusSystemAndMode(cid, fqdn string, services []ServiceHealth, system *SystemMetrics, mode *Mode, status HostStatus) HostHealth {
	// For consistency
	if services == nil {
		services = make([]ServiceHealth, 0)
	}

	return HostHealth{
		cid:      cid,
		fqdn:     fqdn,
		services: services,
		status:   status,
		system:   system,
		mode:     mode,
	}
}

// NewHostHealth constructs host health object
func NewHostHealth(cid, fqdn string, services []ServiceHealth) HostHealth {
	return NewHostHealthWithStatusAndMode(cid, fqdn, services, nil, HostStatusUnknown)
}

// NewHostHealthWithMode constructs host health object
func NewHostHealthWithMode(cid, fqdn string, services []ServiceHealth, mode *Mode) HostHealth {
	return NewHostHealthWithStatusAndMode(cid, fqdn, services, mode, HostStatusUnknown)
}

// NewHostHealthWithSystem constructs host health object
func NewHostHealthWithSystem(cid, fqdn string, services []ServiceHealth, system *SystemMetrics) HostHealth {
	return NewHostHealthWithStatusSystemAndMode(cid, fqdn, services, system, nil, HostStatusUnknown)
}

// NewHostHealthWithSystemAndMode constructs host health object
func NewHostHealthWithSystemAndMode(cid, fqdn string, services []ServiceHealth, system *SystemMetrics, mode *Mode) HostHealth {
	return NewHostHealthWithStatusSystemAndMode(cid, fqdn, services, system, mode, HostStatusUnknown)
}

// NewHostHealthWithSystemAndStatus constructs host health object
func NewHostHealthWithSystemAndStatus(cid, fqdn string, services []ServiceHealth, system *SystemMetrics, status HostStatus) HostHealth {
	return NewHostHealthWithStatusSystemAndMode(cid, fqdn, services, system, nil, status)
}

// NewUnknownHostHealth constructs host health object for unknown host
func NewUnknownHostHealth(fqdn string) HostHealth {
	return HostHealth{
		fqdn:     fqdn,
		status:   HostStatusUnknown,
		services: make([]ServiceHealth, 0),
		system:   nil,
	}
}

// ClusterID ...
func (hh HostHealth) ClusterID() string {
	return hh.cid
}

// FQDN ...
func (hh HostHealth) FQDN() string {
	return hh.fqdn
}

// Status ...
func (hh HostHealth) Status() HostStatus {
	return hh.status
}

// Services ...
func (hh HostHealth) Services() []ServiceHealth {
	return hh.services
}

// System ...
func (hh HostHealth) System() *SystemMetrics {
	return hh.system
}

// Mode ...
func (hh HostHealth) Mode() *Mode {
	return hh.mode
}

// SetHostStatus ...
func (hh *HostHealth) SetHostStatus(status HostStatus) {
	hh.status = status
}

// ServiceStatus ...
type ServiceStatus string

func (ss *ServiceStatus) String() string {
	return string(*ss)
}

// List of possible service statuses
const (
	ServiceStatusAlive   ServiceStatus = "Alive"
	ServiceStatusDead    ServiceStatus = "Dead"
	ServiceStatusUnknown ServiceStatus = "Unknown"
)

var serviceStatusMap = map[ServiceStatus]struct{}{
	ServiceStatusAlive:   {},
	ServiceStatusDead:    {},
	ServiceStatusUnknown: {},
}

// ParseServiceStatus from string
func ParseServiceStatus(str string) ServiceStatus {
	ss := ServiceStatus(str)
	if _, ok := serviceStatusMap[ss]; !ok {
		return ServiceStatusUnknown
	}

	return ss
}

// UnmarshalText unmarshals service status from text
func (ss *ServiceStatus) UnmarshalText(text []byte) error {
	*ss = ParseServiceStatus(string(text))
	return nil
}

// ClusterHealth defines status of cluster
type ClusterHealth struct {
	Cid             string
	Env             string
	SLA             bool
	UserFaultBroken bool
	Nonaggregatable bool
	Status          ClusterStatus
	StatusTS        time.Time
	AliveTS         time.Time
}

// DBRWInfo information about Read\Write cluster\shard availability, where DB.... is cluster or shard
type DBRWInfo struct {
	HostsTotal int
	HostsRead  int
	HostsWrite int
	DBTotal    int
	DBRead     int
	DBWrite    int

	HostsBrokenByUser int
	// DBBroken marks that cluster (or shard) broken by a user
	// Keep in mind that the broken DB shouldn't have RW metrics
	// https://a.yandex-team.ru/arc/commit/r7450077
	DBBroken int
}

func (dbrw *DBRWInfo) Add(other DBRWInfo) DBRWInfo {
	return DBRWInfo{
		HostsTotal: dbrw.HostsTotal + other.HostsTotal,
		HostsRead:  dbrw.HostsRead + other.HostsRead,
		HostsWrite: dbrw.HostsWrite + other.HostsWrite,
		DBTotal:    dbrw.DBTotal + other.DBTotal,
		DBRead:     dbrw.DBRead + other.DBRead,
		DBWrite:    dbrw.DBWrite + other.DBWrite,
	}
}

// NewUnknownClusterHealth create unknown cluster health state
func NewUnknownClusterHealth(cid string) ClusterHealth {
	return ClusterHealth{
		Cid:    cid,
		Status: ClusterStatusUnknown,
	}
}

// SupportTopology return true if topology supported by MDB Health service
func SupportTopology(ctype metadb.ClusterType) bool {
	switch ctype {
	case metadb.MongodbCluster:
	case metadb.ClickhouseCluster:
	case metadb.MysqlCluster:
	case metadb.RedisCluster:
	case metadb.PostgresqlCluster:
	case metadb.ElasticSearchCluster:
	case metadb.KafkaCluster:
	case metadb.SQLServerCluster:
	case metadb.GreenplumCluster:
	default:
		return false
	}
	return true
}

// HostStatus ...
type HostStatus string

// List of possible cluster statuses
const (
	HostStatusAlive    HostStatus = "Alive"
	HostStatusDead     HostStatus = "Dead"
	HostStatusUnknown  HostStatus = "Unknown"
	HostStatusDegraded HostStatus = "Degraded"
)

// ClusterStatus ...
type ClusterStatus string

// List of possible cluster statuses
const (
	ClusterStatusAlive    ClusterStatus = "Alive"
	ClusterStatusDead     ClusterStatus = "Dead"
	ClusterStatusUnknown  ClusterStatus = "Unknown"
	ClusterStatusDegraded ClusterStatus = "Degraded"
)

// ServiceRole ...
type ServiceRole string

func (ss ServiceRole) String() string {
	return string(ss)
}

// List of possible service roles
const (
	ServiceRoleMaster  ServiceRole = "Master"
	ServiceRoleReplica ServiceRole = "Replica"
	ServiceRoleUnknown ServiceRole = "Unknown"
)

// ServiceReplicaType ...
type ServiceReplicaType string

// List of possible service replica types
const (
	ServiceReplicaTypeUnknown ServiceReplicaType = "Unknown"
	ServiceReplicaTypeAsync   ServiceReplicaType = "Async"
	ServiceReplicaTypeSync    ServiceReplicaType = "Sync"
	ServiceReplicaTypeQuorum  ServiceReplicaType = "Quorum"
)

var TrackedSLACLusterStatuses = []string{
	metadb.ClusterStatusRunning,
	metadb.ClusterStatusModifyError,
	metadb.ClusterStatusStopError,
	metadb.ClusterStatusStartError,
	metadb.ClusterStatusRestoreOnlineError,
}

// ParseServiceReplicaType convert to service replica type or unknown
func ParseServiceReplicaType(str string) ServiceReplicaType {
	switch str {
	case string(ServiceReplicaTypeAsync):
		return ServiceReplicaTypeAsync
	case string(ServiceReplicaTypeSync):
		return ServiceReplicaTypeSync
	case string(ServiceReplicaTypeQuorum):
		return ServiceReplicaTypeQuorum
	default:
		return ServiceReplicaTypeUnknown
	}
}

// ServiceHealth defines status and metrics for one service of one host of cluster
type ServiceHealth struct {
	name            string
	ts              time.Time
	status          ServiceStatus
	role            ServiceRole
	replicaType     ServiceReplicaType
	replicaUpstream string
	replicaLag      int64
	metrics         map[string]string
}

// NewServiceHealth constructs host health object
func NewServiceHealth(
	name string,
	ts time.Time,
	status ServiceStatus,
	role ServiceRole,
	replicaType ServiceReplicaType,
	replicaUpstream string,
	replicaLag int64,
	metrics map[string]string,
) ServiceHealth {
	// For consistency
	if metrics == nil {
		metrics = make(map[string]string)
	}

	return ServiceHealth{
		name:        name,
		ts:          ts.Truncate(time.Second).UTC(),
		status:      status,
		role:        role,
		replicaType: replicaType,
		metrics:     metrics,
	}
}

// NewUnknownServiceHealth constructs service health object for unknown service
func NewUnknownServiceHealth() ServiceHealth {
	return ServiceHealth{
		status:      ServiceStatusUnknown,
		role:        ServiceRoleUnknown,
		replicaType: ServiceReplicaTypeUnknown,
		metrics:     make(map[string]string),
	}
}

// NewUnknownHealthForService constructs service unknown health object for service
func NewUnknownHealthForService(service string) ServiceHealth {
	return ServiceHealth{
		name:        service,
		status:      ServiceStatusUnknown,
		role:        ServiceRoleUnknown,
		replicaType: ServiceReplicaTypeUnknown,
		metrics:     make(map[string]string),
	}
}

// NewAliveHealthForService constructs alive service health
// with unknown rol and unknown replica type
func NewAliveHealthForService(name string) ServiceHealth {
	return NewServiceHealth(
		name,
		time.Now(),
		ServiceStatusAlive,
		ServiceRoleUnknown,
		ServiceReplicaTypeUnknown,
		"",
		0,
		nil,
	)
}

// NewDeadHealthForService constructs dead service health
// with unknown rol and unknown replica type
func NewDeadHealthForService(name string) ServiceHealth {
	return NewServiceHealth(
		name,
		time.Now(),
		ServiceStatusDead,
		ServiceRoleUnknown,
		ServiceReplicaTypeUnknown,
		"",
		0,
		nil,
	)
}

// Name ...
func (sh ServiceHealth) Name() string {
	return sh.name
}

// Timestamp ...
func (sh ServiceHealth) Timestamp() time.Time {
	return sh.ts
}

// Status ...
func (sh ServiceHealth) Status() ServiceStatus {
	return sh.status
}

// Role ...
func (sh ServiceHealth) Role() ServiceRole {
	return sh.role
}

// ReplicaType ...
func (sh ServiceHealth) ReplicaType() ServiceReplicaType {
	return sh.replicaType
}

// ReplicaUpstream ...
func (sh ServiceHealth) ReplicaUpstream() string {
	return sh.replicaUpstream
}

// ReplicaLag ...
func (sh ServiceHealth) ReplicaLag() int64 {
	return sh.replicaLag
}

// Metrics ...
func (sh ServiceHealth) Metrics() map[string]string {
	return sh.metrics
}

type ClusterRoles struct {
	clusters map[string][]string // cid -> roles
}

func (cr *ClusterRoles) Add(cid, role string) {
	_, ok := cr.clusters[cid]
	if !ok {
		cr.clusters[cid] = []string{role}
	} else {
		cr.clusters[cid] = append(cr.clusters[cid], role)
	}
}
