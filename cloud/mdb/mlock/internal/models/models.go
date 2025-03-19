package models

import "time"

// LockID is a simple string alias for type safety
type LockID string

// LockHolder is a simple string alias for type safety
type LockHolder string

// LockReason is a simple string alias for type safety
type LockReason string

// LockObject is a simple string alias for type safety
type LockObject string

// LockConflict represents single object conflict
type LockConflict struct {
	Object LockObject
	Ids    []LockID
}

// Lock represents mlock without status information
type Lock struct {
	ID       LockID
	Holder   LockHolder
	Reason   LockReason
	Objects  []LockObject
	CreateTS time.Time
}

// LockStatus represents mlock with status
type LockStatus struct {
	ID        LockID
	Holder    LockHolder
	Reason    LockReason
	Objects   []LockObject
	CreateTS  time.Time
	Acquired  bool
	Conflicts []LockConflict
}
