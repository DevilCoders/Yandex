package locker

import "context"

//go:generate ../../../../../scripts/mockgen.sh Locker

type Locker interface {
	Acquire(ctx context.Context, lockID string, fqdns []string, reason string) error
	Release(ctx context.Context, lockID string) error
}
