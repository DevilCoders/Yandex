package mlock_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	mlocker "a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/locker/mlock"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	lockID = "test-lock-id"
	reason = "lock reason"
	holder = "katan"
)

var lockObjects = []string{"FQDN"}

func TestLocker(t *testing.T) {

	t.Run("CreateLock and immediate acquire it", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		m := mocks.NewMockLocker(mockCtrl)

		m.EXPECT().CreateLock(gomock.Any(), lockID, holder, lockObjects, reason).
			Return(nil)
		m.EXPECT().GetLockStatus(gomock.Any(), lockID).
			Return(mlockclient.LockStatus{Acquired: true}, nil)

		lck := mlocker.New(mlocker.DefaultConfig(), m, &nop.Logger{})
		require.NoError(t, lck.Acquire(context.Background(), lockID, lockObjects, reason))
	})

	t.Run("Create and Acquire it on second iteration", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		m := mocks.NewMockLocker(mockCtrl)
		cfg := mlocker.DefaultConfig()
		cfg.Retries.InitialInterval = time.Nanosecond
		cfg.CheckInterval = encodingutil.FromDuration(time.Nanosecond)

		m.EXPECT().CreateLock(gomock.Any(), lockID, holder, lockObjects, reason).
			Return(semerr.Unavailable(""))
		m.EXPECT().CreateLock(gomock.Any(), lockID, holder, lockObjects, reason).
			Return(nil)
		m.EXPECT().GetLockStatus(gomock.Any(), lockID).
			Return(mlockclient.LockStatus{}, semerr.Unavailable(""))
		m.EXPECT().GetLockStatus(gomock.Any(), lockID).
			Return(mlockclient.LockStatus{Acquired: false}, nil)
		m.EXPECT().GetLockStatus(gomock.Any(), lockID).
			Return(mlockclient.LockStatus{Acquired: true}, nil)

		lck := mlocker.New(cfg, m, &nop.Logger{})
		require.NoError(t, lck.Acquire(context.Background(), lockID, lockObjects, reason))
	})

	t.Run("Do Release if Created failed", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		m := mocks.NewMockLocker(mockCtrl)
		cfg := mlocker.DefaultConfig()
		cfg.Retries.InitialInterval = time.Nanosecond
		cfg.Retries.MaxRetries = 1

		m.EXPECT().CreateLock(gomock.Any(), lockID, holder, lockObjects, reason).
			AnyTimes().
			Return(semerr.Unavailable(""))
		m.EXPECT().ReleaseLock(gomock.Any(), lockID).
			Return(nil)

		lck := mlocker.New(cfg, m, &nop.Logger{})
		require.Error(t, lck.Acquire(context.Background(), lockID, lockObjects, reason))
	})

	t.Run("do Release if Create panics", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		m := mocks.NewMockLocker(mockCtrl)
		cfg := mlocker.DefaultConfig()

		m.EXPECT().CreateLock(gomock.Any(), lockID, holder, lockObjects, reason).
			Do(func(_, _, _, _, _ interface{}) { panic(xerrors.New("something ORLY bad happens")) })
		m.EXPECT().ReleaseLock(gomock.Any(), lockID).
			Return(nil)

		lck := mlocker.New(cfg, m, &nop.Logger{})
		require.Panics(t, func() { _ = lck.Acquire(context.Background(), lockID, lockObjects, reason) })
	})

	t.Run("do Release if GetStatus panics", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		m := mocks.NewMockLocker(mockCtrl)
		cfg := mlocker.DefaultConfig()

		m.EXPECT().
			CreateLock(gomock.Any(), lockID, holder, lockObjects, reason).
			Return(nil)
		m.EXPECT().GetLockStatus(gomock.Any(), gomock.Any()).
			Do(func(_, _ interface{}) { panic(xerrors.New("something ORLY bad happens")) })
		m.EXPECT().ReleaseLock(gomock.Any(), lockID).
			Return(nil)

		lck := mlocker.New(cfg, m, &nop.Logger{})
		require.Panics(t, func() { _ = lck.Acquire(context.Background(), lockID, lockObjects, reason) })
	})

	t.Run("Created by not Acquire in Deadline", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		m := mocks.NewMockLocker(mockCtrl)
		cfg := mlocker.DefaultConfig()
		cfg.Retries.InitialInterval = time.Nanosecond
		cfg.CheckInterval = encodingutil.FromDuration(time.Nanosecond)

		m.EXPECT().
			CreateLock(gomock.Any(), lockID, holder, lockObjects, reason).
			Return(nil)
		m.EXPECT().GetLockStatus(gomock.Any(), lockID).
			AnyTimes().
			Return(mlockclient.LockStatus{Acquired: false}, nil)
		m.EXPECT().ReleaseLock(gomock.Any(), lockID).
			Return(nil)

		lck := mlocker.New(cfg, m, &nop.Logger{})
		require.Error(t, lck.Acquire(context.Background(), lockID, lockObjects, reason))
	})

	t.Run("Release for NOT_FOUND is ok", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		m := mocks.NewMockLocker(mockCtrl)

		m.EXPECT().
			ReleaseLock(gomock.Any(), lockID).
			Return(semerr.NotFound("it not found"))

		lck := mlocker.New(mlocker.DefaultConfig(), m, &nop.Logger{})
		require.NoError(t, lck.Release(context.Background(), lockID))
	})

	t.Run("Release retry errors", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		m := mocks.NewMockLocker(mockCtrl)
		cfg := mlocker.DefaultConfig()
		cfg.Retries.MaxRetries = 2
		cfg.Retries.InitialInterval = time.Nanosecond

		m.EXPECT().ReleaseLock(gomock.Any(), lockID).
			Times(3).
			Return(xerrors.New("test error"))

		lck := mlocker.New(cfg, m, &nop.Logger{})
		require.Error(t, lck.Release(context.Background(), lockID))
	})
}
