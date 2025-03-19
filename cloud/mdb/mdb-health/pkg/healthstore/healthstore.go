package healthstore

import (
	"context"
	"io"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

//go:generate ../../../scripts/mockgen.sh Backend

type HostHealthToStore struct {
	Health types.HostHealth
	TTL    time.Duration
}

type Backend interface {
	io.Closer
	ready.Checker

	Name() string

	StoreHostsHealth(ctx context.Context, hostsHealth []HostHealthToStore) error
}
