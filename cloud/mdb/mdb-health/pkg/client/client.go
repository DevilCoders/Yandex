package client

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh MDBHealthClient

// Public errors
var (
	ErrInternalError = xerrors.NewSentinel("internal server error")
	ErrNotFound      = xerrors.NewSentinel("not found")
	ErrBadRequest    = xerrors.NewSentinel("bad request")
)

type ClusterStatus string

const (
	ClusterStatusAlive    ClusterStatus = "Alive"
	ClusterStatusDead     ClusterStatus = "Dead"
	ClusterStatusUnknown  ClusterStatus = "Unknown"
	ClusterStatusDegraded ClusterStatus = "Degraded"
)

type ClusterHealth struct {
	Status             ClusterStatus
	Timestamp          time.Time
	LastAliveTimestamp time.Time
}

// MDBHealthClient is an interface to mdb-health service
type MDBHealthClient interface {
	// Ping queries mdb-health status
	Ping(ctx context.Context) error
	// Stats queries mdb-health stats
	Stats(ctx context.Context) error

	// GetHostsHealth queries mdb-health for health of specified fqdns
	GetHostsHealth(ctx context.Context, fqdns []string) ([]types.HostHealth, error)
	// UpdateHostHealth updates health of host using provided key to sign it
	UpdateHostHealth(ctx context.Context, hh types.HostHealth, key *crypto.PrivateKey) error
	// GetClusterHealth queries mdb-health for health of specified cluster
	GetClusterHealth(ctx context.Context, cid string) (ClusterHealth, error)
	GetHostNeighboursInfo(ctx context.Context, Fqdns []string) (map[string]types.HostNeighboursInfo, error)
}
