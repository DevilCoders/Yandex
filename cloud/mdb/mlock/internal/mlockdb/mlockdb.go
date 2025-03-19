package mlockdb

import (
	"context"
	"errors"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/models"
)

// Database errors
var (
	ErrNotAvailable = errors.New("database not available")
	ErrNotFound     = errors.New("not found")
	ErrConflict     = errors.New("conflict")
)

// MlockDB is a database interface
type MlockDB interface {
	ready.Checker

	sqlutil.TxBinder

	CreateLock(ctx context.Context, lock models.Lock) error
	ReleaseLock(ctx context.Context, lockID models.LockID) error
	GetLocks(ctx context.Context, holder models.LockHolder, limit int64, offset int64) ([]models.Lock, bool, error)
	GetLockStatus(ctx context.Context, lockID models.LockID) (models.LockStatus, error)
}
