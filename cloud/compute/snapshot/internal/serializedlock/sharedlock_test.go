package serializedlock

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestClean(t *testing.T) {
	a := assert.New(t)
	now := time.Date(2000, 1, 1, 3, 0, 0, 0, time.UTC)

	l := Lock{
		Locks: map[LockIDType]int64{
			"1": time.Date(2000, 1, 1, 1, 0, 0, 0, time.UTC).UnixNano(),
			"2": time.Date(2000, 1, 1, 3, 0, 0, 0, time.UTC).UnixNano(),
		},
	}
	l.clean(now.UnixNano())
	a.Empty(l.Locks)

	l = Lock{
		Locks: map[LockIDType]int64{
			"1": time.Date(2000, 1, 1, 1, 0, 0, 0, time.UTC).UnixNano(),
			"2": time.Date(2000, 1, 1, 3, 0, 0, 1, time.UTC).UnixNano(),
		},
	}
	l.clean(now.UnixNano())
	a.Equal(l.Locks, map[LockIDType]int64{
		"2": time.Date(2000, 1, 1, 3, 0, 0, 1, time.UTC).UnixNano(),
	})
}

func TestIsLocked(t *testing.T) {
	a := assert.New(t)

	now := time.Date(2000, 1, 1, 3, 0, 0, 0, time.UTC)

	l := Lock{}
	a.False(l.IsLocked(now))

	l = Lock{
		Locks: map[LockIDType]int64{
			"1": time.Date(2000, 1, 1, 1, 0, 0, 0, time.UTC).UnixNano(),
		},
	}
	a.False(l.IsLocked(now))

	l = Lock{
		Locks: map[LockIDType]int64{
			"1": time.Date(2000, 1, 1, 1, 0, 0, 0, time.UTC).UnixNano(),
			"2": time.Date(2000, 1, 1, 3, 0, 0, 0, time.UTC).UnixNano(),
		},
	}
	a.False(l.IsLocked(now))

	l = Lock{
		Locks: map[LockIDType]int64{
			"1": time.Date(2000, 1, 1, 3, 0, 0, 1, time.UTC).UnixNano(),
		},
	}
	a.True(l.IsLocked(now))
}

func TestLock(t *testing.T) {
	a := assert.New(t)

	now := time.Date(2000, 1, 1, 3, 0, 0, 0, time.UTC)

	// simple lock
	l := Lock{}
	lockID, err := l.Lock(Exclusive, now, now.Add(time.Hour))
	a.NoError(err)
	a.NotEmpty(lockID)
	a.Equal(Exclusive, l.Type)
	a.Equal(map[LockIDType]int64{lockID: now.Add(time.Hour).UnixNano()}, l.Locks)

	// Lock after expired lock
	l = Lock{
		Locks: map[LockIDType]int64{
			"1": now.UnixNano(),
		},
	}
	lockID, err = l.Lock(Exclusive, now, now.Add(time.Hour))
	a.NoError(err)
	a.NotEmpty(lockID)
	a.Equal(Exclusive, l.Type)
	a.Equal(map[LockIDType]int64{lockID: now.Add(time.Hour).UnixNano()}, l.Locks)

	// Lock after not expired lock
	l = Lock{
		Locks: map[LockIDType]int64{
			"1": time.Date(2000, 1, 1, 4, 0, 0, 0, time.UTC).UnixNano(),
		},
	}
	lockID, err = l.Lock(Exclusive, now, now.Add(time.Hour))
	a.Equal(err, ErrAlreadyLocked)
	a.Empty(lockID)
	lockID, err = l.Lock(Shared, now, now.Add(time.Hour))
	a.Equal(err, ErrAlreadyLocked)
	a.Empty(lockID)
	a.Empty(l.Type)
	a.Equal(map[LockIDType]int64{"1": time.Date(2000, 1, 1, 4, 0, 0, 0, time.UTC).UnixNano()}, l.Locks)

	// Lock after exclusive lock
	l = Lock{}
	lockID, err = l.Lock(Exclusive, now, now.Add(time.Hour))
	a.NoError(err)
	a.NotEmpty(lockID)
	a.Equal(Exclusive, l.Type)
	a.Equal(map[LockIDType]int64{lockID: now.Add(time.Hour).UnixNano()}, l.Locks)
	lockID, err = l.Lock(Exclusive, now, now.Add(time.Hour))
	a.Equal(err, ErrAlreadyLocked)
	a.Empty(lockID)
	a.Equal(Exclusive, l.Type)
	lockID, err = l.Lock(Shared, now, now.Add(time.Hour))
	a.Equal(err, ErrAlreadyLocked)
	a.Empty(lockID)
	a.Equal(Exclusive, l.Type)

	// Lock after shared lock
	l = Lock{}
	lockID, err = l.Lock(Shared, now, now.Add(time.Hour))
	a.NoError(err)
	a.NotEmpty(lockID)
	a.Equal(Shared, l.Type)
	a.Equal(map[LockIDType]int64{lockID: now.Add(time.Hour).UnixNano()}, l.Locks)
	lockID2, err := l.Lock(Exclusive, now, now.Add(time.Hour))
	a.Equal(err, ErrAlreadyLocked)
	a.Empty(lockID2)
	a.Equal(Shared, l.Type)
	lockID3, err := l.Lock(Shared, now, now.Add(time.Minute))
	a.NoError(err)
	a.NotEmpty(lockID3)
	a.NotEqual(lockID, lockID3)
	a.Equal(Shared, l.Type)
	a.Equal(map[LockIDType]int64{
		lockID:  now.Add(time.Hour).UnixNano(),
		lockID3: now.Add(time.Minute).UnixNano(),
	}, l.Locks)

	// Bad lock type
	l = Lock{
		Locks: map[LockIDType]int64{
			"1": time.Date(2000, 1, 1, 1, 0, 0, 0, time.UTC).UnixNano(),
		},
	}
	lockID, err = l.Lock("", now, now.Add(time.Hour))
	a.Equal(ErrBadLockType, err)
	a.Empty(lockID)
	a.NotEmpty(l.Locks)
}

func TestUnlock(t *testing.T) {
	a := assert.New(t)

	now := time.Date(2000, 1, 1, 3, 0, 0, 0, time.UTC)

	// unlock exclusive
	l := Lock{
		Type: Exclusive,
		Locks: map[LockIDType]int64{
			"123": now.Add(time.Hour).UnixNano(),
		},
	}
	err := l.Unlock("123", now)
	a.NoError(err)
	a.Empty(l.Locks)

	// unlock shared
	l = Lock{
		Type: Shared,
		Locks: map[LockIDType]int64{
			"123": now.Add(time.Hour).UnixNano(),
		},
	}
	err = l.Unlock("123", now)
	a.NoError(err)
	a.Empty(l.Locks)

	// unlock shared partial
	l = Lock{
		Type: Shared,
		Locks: map[LockIDType]int64{
			"123": now.Add(time.Hour).UnixNano(),
			"222": now.Add(time.Hour).UnixNano(),
		},
	}
	err = l.Unlock("123", now)
	a.NoError(err)
	a.Equal(map[LockIDType]int64{
		"222": now.Add(time.Hour).UnixNano(),
	}, l.Locks)

	// unlock empty
	l = Lock{}
	err = l.Unlock("", now)
	a.Equal(ErrDoesntLocked, err)

	// unlock with bad id
	l = Lock{
		Type: Shared,
		Locks: map[LockIDType]int64{
			"123": now.Add(time.Hour).UnixNano(),
		},
	}
	err = l.Unlock("222", now)
	a.Equal(ErrBadLockID, err)

	// unlock expired
	l = Lock{
		Type: Shared,
		Locks: map[LockIDType]int64{
			"123": now.Add(-time.Hour).UnixNano(),
		},
	}
	err = l.Unlock("123", now)
	a.Equal(ErrDoesntLocked, err)
	a.Empty(l.Locks)
}
