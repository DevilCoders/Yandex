package dbm

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../scripts/mockgen.sh Client

var (
	ErrUnavailable = xerrors.NewSentinel("dbm is unavailable")
	ErrBadRequest  = xerrors.NewSentinel("bad request")
	ErrMissing     = xerrors.NewSentinel("missing")
)

// Container in terms of DBM
type Container struct {
	ClusterName string
	FQDN        string
	Dom0        string
}

type NewHostsInfo struct {
	SetBy           string
	NewHostsAllowed bool
}

// Client is DBM client
type Client interface {
	// Dom0Containers return containers on given dom0
	Dom0Containers(ctx context.Context, dom0 string) ([]Container, error)
	// ClusterContainers return containers for given cluster by it's name
	ClusterContainers(ctx context.Context, clusterName string) ([]Container, error)
	VolumesByDom0(ctx context.Context, dom0 string) (map[string]ContainerVolumes, error)
	AreNewHostsAllowed(ctx context.Context, dom0 string) (NewHostsInfo, error)
	UpdateNewHostsAllowed(ctx context.Context, dom0 string, allowNewHosts bool) error
}
