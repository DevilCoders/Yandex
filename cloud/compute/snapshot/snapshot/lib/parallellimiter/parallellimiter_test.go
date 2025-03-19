package parallellimiter

import (
	"context"
	"testing"

	"golang.org/x/xerrors"

	"github.com/stretchr/testify/require"

	"github.com/jonboulle/clockwork"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

type LockHolderMock string

func (l *LockHolderMock) Close(ctx context.Context) {
	s := string(*l) + " (closed)"
	*l = LockHolderMock(s)
}

func NewLockHolderMock(s string) *LockHolderMock {
	lh := LockHolderMock(s)
	return &lh
}

type LockerMock struct {
	holders map[string]storage.LockHolder
	mock.Mock
}

func (lm *LockerMock) LockSnapshotSharedMax(_ context.Context, lockID string, reason string, maxActiveLocks int) (lockedCTX context.Context, holder storage.LockHolder, resErr error) {
	args := lm.Called(nil, lockID, reason, maxActiveLocks)
	if args.Get(0) != nil {
		lockedCTX = args.Get(0).(context.Context)
	}
	if args.Get(1) != nil {
		holder = args.Get(1).(storage.LockHolder)
	}
	if args.Get(2) != nil {
		resErr = args.Get(2).(error)
	}

	if lm.holders == nil {
		lm.holders = make(map[string]storage.LockHolder)
	}
	lm.holders[lockID] = holder
	return
}

func TestLimiterGetLocks(t *testing.T) {
	lockerMock := &LockerMock{}
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	resultContext1, cancel1 := context.WithCancel(ctx)
	defer cancel1()

	rt := require.New(t)

	fc := clockwork.NewFakeClock()

	rl := RateLimiter{
		locker: lockerMock,
		t:      fc,
	}

	// Test without limits - no locks get
	resCtx, holder, resErr := rl.getLocks(ctx)
	rt.Equal(ctx, resCtx)
	rt.NoError(resErr)
	rt.Len(holder, 0)

	// Test without limits - inactive lock
	resCtx, holder, resErr = rl.getLocks(ctx, lockDescriptor{Active: false})
	rt.Equal(ctx, resCtx)
	rt.NoError(resErr)
	rt.Len(holder, 0)

	// Get one lock
	holderName := "holder-name"
	lockName := "lockName"
	lockerMock.On("LockSnapshotSharedMax", nil, "lockId-1", lockName, 1).Return(
		resultContext1, NewLockHolderMock(holderName), nil,
	)
	resCtx, holder, resErr = rl.getLocks(ctx, lockDescriptor{
		Active:   true,
		Name:     lockName,
		LockID:   "lockId-1",
		MaxCount: 1,
	})
	rt.NoError(resErr)
	rt.Equal(resultContext1, resCtx)
	rt.Equal(orderedLocker{NewLockHolderMock(holderName)}, holder)

	// Two locks
	lockerMock.On("LockSnapshotSharedMax", nil, "lockId-Two-1", "reason-1", 1).Return(
		resultContext1, NewLockHolderMock("holder-2-1"), nil,
	)
	lockerMock.On("LockSnapshotSharedMax", nil, "lockId-Two-2", "reason-2", 1).Return(
		resultContext1, NewLockHolderMock("holder-2-2"), nil,
	)
	resCtx, holder, resErr = rl.getLocks(ctx,
		lockDescriptor{
			Active:   true,
			Name:     "reason-1",
			LockID:   "lockId-Two-1",
			MaxCount: 1,
		},
		lockDescriptor{
			Active:   true,
			Name:     "reason-2",
			LockID:   "lockId-Two-2",
			MaxCount: 1,
		},
	)
	rt.NoError(resErr)
	rt.Equal(resultContext1, resCtx)
	rt.Equal(orderedLocker{NewLockHolderMock("holder-2-1"), NewLockHolderMock("holder-2-2")}, holder)

	// Skip one
	lockerMock.On("LockSnapshotSharedMax", nil, "lockId-skip-1", "reason-1", 1).Return(
		resultContext1, NewLockHolderMock("holder-skip-1"), nil,
	)
	lockerMock.On("LockSnapshotSharedMax", nil, "lockId-skip-3", "reason-3", 1).Return(
		resultContext1, NewLockHolderMock("holder-skip-3"), nil,
	)
	resCtx, holder, resErr = rl.getLocks(ctx,
		lockDescriptor{
			Active:   true,
			Name:     "reason-1",
			LockID:   "lockId-skip-1",
			MaxCount: 1,
		},
		lockDescriptor{
			Active:   false,
			Name:     "reason-2",
			LockID:   "lockId-skip-2",
			MaxCount: 1,
		},
		lockDescriptor{
			Active:   true,
			Name:     "reason-3",
			LockID:   "lockId-skip-3",
			MaxCount: 1,
		},
	)
	rt.NoError(resErr)
	rt.Equal(resultContext1, resCtx)
	rt.Equal(orderedLocker{NewLockHolderMock("holder-skip-1"), NewLockHolderMock("holder-skip-3")}, holder)

	// error
	testErr := xerrors.New("test-err")
	lockerMock.On("LockSnapshotSharedMax", nil, "lockId-err-1", "reason-1", 1).Return(
		resultContext1, NewLockHolderMock("holder-err-1"), nil,
	)
	lockerMock.On("LockSnapshotSharedMax", nil, "lockId-err-2", "reason-2", 1).Return(
		resultContext1, nil, testErr,
	)
	resCtx, holder, resErr = rl.getLocks(ctx,
		lockDescriptor{
			Active:   true,
			Name:     "reason-1",
			LockID:   "lockId-err-1",
			MaxCount: 1,
		},
		lockDescriptor{
			Active:   true,
			Name:     "reason-2",
			LockID:   "lockId-err-2",
			MaxCount: 1,
		},
		lockDescriptor{
			Active:   true,
			Name:     "reason-3",
			LockID:   "lockId-err-3",
			MaxCount: 1,
		},
	)
	rt.EqualError(resErr, testErr.Error())
	rt.Nil(resCtx)
	rt.Nil(holder)
}

func TestMakeLock(t *testing.T) {
	rt := require.New(t)

	lock := makeLockDescriptor(0, "asd", "asd")
	rt.False(lock.Active)

	lock = makeLockDescriptor(1, "asd")
	rt.False(lock.Active)

	lock = makeLockDescriptor(1, "asd", "ddd")
	rt.Equal(lockDescriptor{
		Active:   true,
		Name:     "asd",
		LockID:   "ddd-asd-" + lockSuffix,
		MaxCount: 1,
	}, lock)

	lock = makeLockDescriptor(2, "asd", "ddd", "ggg")
	rt.Equal(lockDescriptor{
		Active:   true,
		Name:     "asd",
		LockID:   "ddd-ggg-asd-" + lockSuffix,
		MaxCount: 2,
	}, lock)
}
