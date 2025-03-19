package mlockclient

import (
	"context"
)

//go:generate ../../../scripts/mockgen.sh Locker

type Locker interface {
	CreateLock(ctx context.Context, id, holder string, fqdns []string, reason string) error
	GetLockStatus(ctx context.Context, id string) (LockStatus, error)
	ReleaseLock(ctx context.Context, lockID string) error
}

type Conflict struct {
	Object  string
	LockIDs []string
}

type LockStatus struct {
	ID        string
	Acquired  bool
	Conflicts []Conflict
	Holder    string
	Reason    string
}
