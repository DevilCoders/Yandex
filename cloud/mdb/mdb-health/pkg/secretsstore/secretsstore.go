// Package secretsstore TODO: rename it! move to other place?
package secretsstore

import (
	"context"
	"io"
	"sync"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh Backend

// Known errors
var (
	ErrSecretNotFound = xerrors.NewSentinel("secret not found for cid")
)

// Backend interface to secrets store
type Backend interface {
	io.Closer
	ready.Checker

	// LoadClusterSecret loads cluster secret from secrets store
	LoadClusterSecret(ctx context.Context, cid string) ([]byte, error)
}

type factoryFunc func(logger log.Logger) (Backend, error)

var (
	backendsMu sync.RWMutex
	backends   = make(map[string]factoryFunc)
)

// RegisterBackend registers new secretsstore backend named 'name'
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

// Open constructs new secretsstore backend named 'name'
func Open(name string, logger log.Logger) (Backend, error) {
	backendsMu.RLock()
	backend, ok := backends[name]
	backendsMu.RUnlock()
	if !ok {
		return nil, xerrors.Errorf("unknown backend: %s", name)
	}

	return backend(logger)
}
