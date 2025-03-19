package locks

import (
	"context"

	ydblock "a.yandex-team.ru/cloud/bootstrap/ydb-lock"
)

type Locker interface {
	Init(ctx context.Context) error
	CreateLock(
		ctx context.Context,
		description string,
		hosts []string,
		hbTimeout uint64,
		lockType ydblock.HostLockType,
	) (*ydblock.Lock, error)
	CheckHostLock(ctx context.Context, hostname string, lock *ydblock.Lock) error
	ExtendLock(ctx context.Context, lock *ydblock.Lock) (*ydblock.Lock, error)
	ReleaseLock(ctx context.Context, lock *ydblock.Lock) error
}
