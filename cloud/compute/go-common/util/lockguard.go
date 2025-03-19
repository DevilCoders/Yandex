package util

import "sync"

// LockGuard use only with defer single thread
// Example:
// guard := MakeLockGuard(mutex)
// defer guard.UnlockIfLocked()
//
// guard.Lock()
// err := ...
// if err != nil {
//     return err
// }
// guard.Unlock()
type LockGuard struct {
	lock   sync.Locker
	locked bool
}

func MakeLockGuard(lock sync.Locker) LockGuard {
	return LockGuard{lock: lock}
}

func (l *LockGuard) Lock() {
	l.lock.Lock()
	l.locked = true
}

func (l *LockGuard) Unlock() {
	l.lock.Unlock()
	l.locked = false
}

func (l *LockGuard) IsLocked() bool {
	return l.locked
}

func (l *LockGuard) UnlockIfLocked() {
	if l.locked {
		l.lock.Unlock()
	}
}
