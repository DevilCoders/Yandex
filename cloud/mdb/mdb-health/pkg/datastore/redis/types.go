package redis

import (
	"strconv"
	"time"

	goredis "github.com/go-redis/redis/v8"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type storeServiceHealth struct {
	Timestamp       int64                    `json:"timestamp"`
	Status          types.ServiceStatus      `json:"status"`
	Role            types.ServiceRole        `json:"role"`
	ReplicaType     types.ServiceReplicaType `json:"replicatype"`
	ReplicaUpstream string                   `json:"replica_upstream"`
	ReplicaLag      int64                    `json:"replica_lag"`
	Metrics         map[string]string        `json:"metrics"`
}

type storeMode struct {
	Timestamp       int64 `json:"timestamp"`
	Read            bool  `json:"read"`
	Write           bool  `json:"write"`
	UserFaultBroken bool  `json:"instance_userfault_broken,omitempty"`
}

type storeCPUMetrics struct {
	Timestamp int64   `json:"timestamp"`
	Used      float64 `json:"used"`
}

type storeMemoryMetrics struct {
	Timestamp int64 `json:"timestamp"`
	Used      int64 `json:"used"`
	Total     int64 `json:"total"`
}

type storeDiskMetrics struct {
	Timestamp int64 `json:"timestamp"`
	Used      int64 `json:"used"`
	Total     int64 `json:"total"`
}

type storeAggregateInfo struct {
	Timestamp         int64 `json:"timestamp"`
	Total             int   `json:"total,omitempty"`
	Alive             int   `json:"alive,omitempty"`
	Degraded          int   `json:"degraded,omitempty"`
	Unknown           int   `json:"unknown,omitempty"`
	Dead              int   `json:"dead,omitempty"`
	HostsTotal        int   `json:"hoststotal,omitempty"`
	HostsRead         int   `json:"hostsread,omitempty"`
	HostsWrite        int   `json:"hostswrite,omitempty"`
	HostsBrokenByUser int   `json:"hostsbroken,omitempty"`
	DBTotal           int   `json:"dbtotal,omitempty"`
	DBRead            int   `json:"dbread,omitempty"`
	DBWrite           int   `json:"dbwrite,omitempty"`
	DBBroken          int   `json:"dbbroken,omitempty"`
}

type storeUnhealthyAggregatedInfo struct {
	Count    int      `json:"count,omitempty"`
	Examples []string `json:"examples,omitempty"`
}

type storeUnhealthyRWAggregatedInfo struct {
	Count        int      `json:"count,omitempty"`
	NoReadCount  int      `json:"noreadcount,omitempty"`
	NoWriteCount int      `json:"nowritecount,omitempty"`
	Examples     []string `json:"examples,omitempty"`
}

type parsedClusterInfo struct {
	ctype           metadb.ClusterType
	fqdns           []string
	sla             bool
	slaShards       []string
	nonaggregatable bool
	roleHosts       map[string][]string
	shardHosts      map[string][]string
	geoHosts        map[string][]string
	hostsHealth     map[string][]types.ServiceHealth
	hostsMode       map[string]types.Mode
}

type topologyRev struct {
	ttlReq *goredis.DurationCmd
	revReq *goredis.StringCmd
	rawRev string
	rev    int64
	ttl    time.Duration
}

func (tr *topologyRev) parse() (bool, error) {
	var err error
	tr.rawRev, err = tr.revReq.Result()
	if err != nil {
		return true, err
	}
	tr.rev, err = strconv.ParseInt(tr.rawRev, 10, 0)
	if err != nil {
		return false, err
	}
	tr.ttl, err = tr.ttlReq.Result()
	if err != nil {
		return true, err
	}
	return true, nil
}

func (tr *topologyRev) isActual(rev int64) bool {
	return rev == tr.rev
}

func (tr *topologyRev) needUpdateTTL(minTTL time.Duration) bool {
	return tr.ttl < minTTL
}
