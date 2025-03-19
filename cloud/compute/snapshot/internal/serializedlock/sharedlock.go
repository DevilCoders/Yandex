package serializedlock

import (
	"fmt"
	"time"

	"github.com/gofrs/uuid"
)

const (
	Exclusive LockType = "exclusive"
	Shared    LockType = "shared"
)

var (
	ErrBadLockID     = fmt.Errorf("serializedlock: bad lock id")
	ErrBadLockType   = fmt.Errorf("serializedlock: bad lock type")
	ErrAlreadyLocked = fmt.Errorf("serializedlock: object is locked")
	ErrDoesntLocked  = fmt.Errorf("serializedlock: object is not locked")
)

type LockType string
type LockIDType string

// Lock is lock structure for shared lock over outside ACID storage
// Need read lock state, change, write in single transaction
// work with each instance of Lock must be NOT concurrent
// Field are public for easy serialization only - you MUST NOT use it directly
// Lock takes in account time difference between instances
// It assumes that if current time is T then all other users have time in interval [T-TimeSkew, T+TimeSkew]
// Each user can check if all users see lock in locked/unlocked state depending on that interval (see isLocked isUnlocked)
type Lock struct {
	Type  LockType
	Locks map[LockIDType]int64 // map [lockID] expire time in unix nano

	TimeSkew int64 // unix nano time, represents maximum difference between instances time
}

func (s *Lock) SetSkew(duration time.Duration) {
	s.TimeSkew = duration.Nanoseconds()
}

func (s *Lock) clean(now int64) {
	for id, expire := range s.Locks {
		// drop unlocked blocks (same check as in isUnlock)
		if now >= expire+s.TimeSkew {
			delete(s.Locks, id)
		}
	}

	if len(s.Locks) == 0 {
		s.Type = ""
	}
}

// IsLocked return true if it has any time unexpired lock
func (s *Lock) IsLocked(now time.Time) bool {
	nowNano := now.UnixNano()
	return s.isLocked(nowNano)
}

func (s *Lock) isLocked(now int64) bool {
	for _, expire := range s.Locks {
		// Lock is locked if any blocking is active
		// Blocking considered locked when all users see now < expire.
		// This is the same as if user with most forward time have t_forward < expire, because for any user t <= t_forward
		// If current user have time t, then the most forward user have t <= t_forward <= t + TimeSkew
		// We need to check if t_forward < expire <=> t + TimeSkew < expire <=>  t < expire - TimeSkew
		if now < expire-s.TimeSkew {
			return true
		}
	}
	return false
}

func (s *Lock) isUnlocked(now int64) bool {
	for _, expire := range s.Locks {
		// Lock is unlocked if all blocks are unlocked
		// Blocking considered unlocked when all users see now > expire.
		// This is the same as if user with most behind clock have t_behind > expire, because for any user t >= t_behind
		// If current user have time t, the most behind-clock user have time t_behind and t - TimeSkew < t_slow <= t
		// We need to check if t_behind > expire <=> t - TimeSkew > expire <=>  t > expire + TimeSkew
		if !(now >= expire+s.TimeSkew) {
			return false
		}
	}
	return true
}

func (s *Lock) Lock(t LockType, now, expire time.Time) (LockIDType, error) {
	nowNano, expireNano := now.UnixNano(), expire.UnixNano()
	switch t {
	case Exclusive, Shared:
		// pass
	default:
		return "", ErrBadLockType
	}
	s.clean(nowNano)
	return s.lock(t, nowNano, expireNano)
}

func (s *Lock) lock(t LockType, now, expire int64) (LockIDType, error) {
	locked := s.isLocked(now)
	if locked && !(s.Type == Shared && t == Shared) {
		return "", ErrAlreadyLocked
	}

	lockID := LockIDType(uuid.Must(uuid.NewV4()).String())
	s.Type = t
	if s.Locks == nil {
		s.Locks = make(map[LockIDType]int64)
	}
	s.Locks[lockID] = expire
	return lockID, nil
}

func (s *Lock) Unlock(lockID LockIDType, now time.Time) error {
	nowNano := now.UnixNano()
	s.clean(nowNano)
	return s.unlock(lockID, nowNano)
}

func (s *Lock) unlock(lockID LockIDType, now int64) error {
	if s.isUnlocked(now) {
		return ErrDoesntLocked
	}

	if _, ok := s.Locks[lockID]; !ok {
		return ErrBadLockID
	}
	delete(s.Locks, lockID)
	return nil
}

func (s *Lock) Update(lockID LockIDType, now, expire time.Time) error {
	nowNano, expireNano := now.UnixNano(), expire.UnixNano()
	s.clean(nowNano)
	if !s.isLocked(nowNano) {
		return ErrDoesntLocked
	}
	return s.update(lockID, expireNano)
}

func (s *Lock) update(lockID LockIDType, expire int64) error {
	if _, ok := s.Locks[lockID]; ok {
		s.Locks[lockID] = expire
		return nil
	}
	return ErrBadLockID
}
