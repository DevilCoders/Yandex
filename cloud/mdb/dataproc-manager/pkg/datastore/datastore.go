package datastore

import (
	"context"
	"io"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh Backend

// Storage errors
var (
	ErrNotFound = xerrors.NewSentinel("not found")
)

// Backend describes data storage interface for Dataproc health API
type Backend interface {
	io.Closer

	StoreClusterHealth(ctx context.Context, cid string, report models.ClusterHealth) error
	LoadClusterHealth(ctx context.Context, cid string) (models.ClusterHealth, error)

	StoreHostsHealth(ctx context.Context, cid string, hosts map[string]models.HostHealth) error
	LoadHostsHealth(ctx context.Context, cid string, fqdns []string) (map[string]models.HostHealth, error)

	StoreClusterTopology(ctx context.Context, cid string, topology models.ClusterTopology) error
	GetCachedClusterTopology(ctx context.Context, cid string) (models.ClusterTopology, error)

	StoreDecommissionHosts(ctx context.Context, cid string, decommissionHosts models.DecommissionHosts) error
	DeleteDecommissionHosts(ctx context.Context, cid string) error
	LoadDecommissionHosts(ctx context.Context, cid string) (models.DecommissionHosts, error)
	StoreDecommissionStatus(ctx context.Context, cid string, decommissionStatus models.DecommissionStatus) error
	LoadDecommissionStatus(ctx context.Context, cid string) (models.DecommissionStatus, error)
}
