package datastore

import (
	"context"
	"io"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/unhealthy"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh Backend

const (
	NotAllUpdatedErrText  = "not all updated in redis"
	SecretNotFoundErrText = "secret not found for cid"
)

// Storage errors
var (
	ErrSecretNotFound = xerrors.NewSentinel(SecretNotFoundErrText)
)

// ClusterTopology used for save info about cluster topology
type ClusterTopology struct {
	CID             string
	Rev             int64
	Cluster         metadb.Cluster
	Hosts           []metadb.Host
	Nonaggregatable bool
}

// AggregatedInfo base structure for aggregate cluster information
type AggregatedInfo struct {
	Timestamp       time.Time
	CType           metadb.ClusterType
	AggType         types.AggType
	Env             string
	SLA             bool
	UserFaultBroken bool
	Total           int
	Alive           int
	Degraded        int
	Unknown         int
	Dead            int
	RWInfo          types.DBRWInfo
}

type HostInfo struct {
	Geo             string
	Status          types.HostStatus
	TS              time.Time
	Env             string
	UserFaultBroken bool
	SLA             bool
}

type FewClusterHealthInfo struct {
	Clusters    []types.ClusterHealth
	HostInfo    map[string]HostInfo       // fqdn -> host info (geo, status, sla, broken by usr)
	ClusterInfo map[string]types.DBRWInfo // cid -> DBRWInfo
	NoSLAShards map[string]types.DBRWInfo // sid -> DBRWInfo
	SLAShards   map[string]types.DBRWInfo // sid -> DBRWInfo
	ShardEnv    map[string]string         // sid -> env
	GeoInfo     map[string][]string       // cid -> geos
	NextCursor  string
}

const (
	EndCursor = "0"
)

// Backend interface to main storage
type Backend interface {
	io.Closer
	ready.Checker
	healthstore.Backend

	// api helpers of PUT /v1/hostshealth
	StoreClusterSecret(ctx context.Context, cid string, secret []byte, timeout time.Duration) error
	LoadClusterSecret(ctx context.Context, cid string) ([]byte, error)

	// api
	// PUT /v1/hostshealth
	StoreHostHealth(ctx context.Context, hh types.HostHealth, timeout time.Duration) error
	// POST /v1/listhostshealth GET /v1/hostshealth
	LoadHostsHealth(ctx context.Context, fqdns []string) ([]types.HostHealth, error)
	// GET /v1/hostneighbours
	GetHostNeighboursInfo(ctx context.Context, fqdns []string) (map[string]types.HostNeighboursInfo, error)
	// GET /v1/clusterhealth
	GetClusterHealth(ctx context.Context, cid string) (types.ClusterHealth, error)

	// api (for debug only?)
	LoadUnhealthyAggregatedInfo(ctx context.Context, ctype metadb.ClusterType, agg types.AggType) (unhealthy.UAInfo, error)

	// process SLI
	LoadFewClustersHealth(ctx context.Context, clusterType metadb.ClusterType, cursor string) (FewClusterHealthInfo, error)
	SaveClustersHealth(ctx context.Context, health []types.ClusterHealth, timeout time.Duration) error
	SetAggregateInfo(ctx context.Context, ai AggregatedInfo, ttl time.Duration) error
	SetUnhealthyAggregatedInfo(ctx context.Context, ctype metadb.ClusterType, uai unhealthy.UAInfo, ttl time.Duration) error

	// lead loop
	CaptureTheLead(ctx context.Context, fqdn string, now time.Time, dur time.Duration) (leadFQDN string, until time.Time, err error)
	// RetouchTopology update TTL of actual clusters and return obsolete ones, also set visible=0 if necessary
	RetouchTopology(ctx context.Context, cr []metadb.ClusterRev, ttl time.Duration) (obsolete []metadb.ClusterRev, err error)
	SetClustersTopology(ctx context.Context, topologies []ClusterTopology, ttl time.Duration, mdb metadb.MetaDB) error

	// service loop stats
	LoadAggregateInfo(ctx context.Context) ([]AggregatedInfo, error)
	WriteStats()
}

type factoryFunc func(logger log.Logger) (Backend, error)

var (
	backendsMu sync.RWMutex
	backends   = make(map[string]factoryFunc)
)

// RegisterBackend registers new datastore backend named 'name'
func RegisterBackend(name string, factory factoryFunc) {
	backendsMu.Lock()
	defer backendsMu.Unlock()

	if factory == nil {
		panic("datastore: register backend is nil")
	}

	if _, dup := backends[name]; dup {
		panic("datastore: register backend called twice for driver " + name)
	}

	backends[name] = factory
}

// Open constructs new datastore backend named 'name'
func Open(name string, logger log.Logger) (Backend, error) {
	backendsMu.RLock()
	backend, ok := backends[name]
	backendsMu.RUnlock()
	if !ok {
		return nil, semerr.InvalidInputf("backend %q", name)
	}

	return backend(logger)
}
