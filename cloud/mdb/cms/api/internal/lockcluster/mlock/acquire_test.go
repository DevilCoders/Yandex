package mlock_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery"
	clusterdiscoverymocks "a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster/mlock"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	mlockclientmocks "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestLockCluster(t *testing.T) {
	ctx := context.Background()
	fqdn := "fqdn"
	taskID := "taskID"
	clusterID := "clusterID"
	lockID := "cms-taskID-clusterID"
	tcs := []struct {
		name     string
		prepare  func(m *mlockclientmocks.MockLocker, d *clusterdiscoverymocks.MockOtherLegsDiscovery)
		validate func(t *testing.T, res *lockcluster.State, err error)
		holder   lockcluster.Holder
	}{
		{
			name: "simple lock",
			prepare: func(m *mlockclientmocks.MockLocker, d *clusterdiscoverymocks.MockOtherLegsDiscovery) {
				d.EXPECT().FindInShardOrSubcidByFQDN(gomock.Any(), fqdn).Return(
					clusterdiscovery.Neighbourhood{
						Others: []metadb.Host{{FQDN: "f1"}, {FQDN: "f2"}},
						ID:     clusterID,
					}, nil)
				m.EXPECT().CreateLock(gomock.Any(), lockID, "", []string{"f1", "f2", fqdn}, gomock.Any()).Return(nil)
				m.EXPECT().GetLockStatus(gomock.Any(), lockID).Return(mlockclient.LockStatus{Acquired: true}, nil)
			},
			validate: func(t *testing.T, res *lockcluster.State, err error) {
				require.NoError(t, err)
				require.Equal(t, lockID, res.LockID)
				require.True(t, res.IsTaken)
			},
		},
		{
			name:   "conflict instance",
			holder: lockcluster.InstanceCMS,
			prepare: func(m *mlockclientmocks.MockLocker, d *clusterdiscoverymocks.MockOtherLegsDiscovery) {
				holder := string(lockcluster.InstanceCMS)
				d.EXPECT().FindInShardOrSubcidByFQDN(gomock.Any(), fqdn).Return(
					clusterdiscovery.Neighbourhood{
						Others: []metadb.Host{{FQDN: "f1"}, {FQDN: "f2"}},
						ID:     clusterID,
					}, nil)
				m.EXPECT().CreateLock(gomock.Any(), lockID, holder, []string{"f1", "f2", fqdn}, gomock.Any()).Return(nil)
				m.EXPECT().GetLockStatus(gomock.Any(), lockID).Return(mlockclient.LockStatus{
					Conflicts: []mlockclient.Conflict{
						{
							Object:  fqdn,
							LockIDs: []string{"anotherLock"},
						},
						{
							Object:  "f1",
							LockIDs: []string{"anotherLock2"},
						},
					},
					Holder: holder,
					ID:     lockID,
				}, nil)
				m.EXPECT().GetLockStatus(gomock.Any(), "anotherLock").Return(mlockclient.LockStatus{
					Acquired: true,
					Holder:   "anotherholder",
				}, nil)
			},
			validate: func(t *testing.T, res *lockcluster.State, err error) {
				require.Error(t, err)
				require.True(t, xerrors.Is(err, lockcluster.NotAcquiredConflicts))
				reason, err := lockcluster.ErrorReason(err)
				require.True(t, xerrors.Is(err, lockcluster.NotAcquiredConflicts))
				require.Equal(t, "could not acquire lock \"cms-taskID-clusterID\""+
					" because of conflicts: fqdn locked by anotherLock, f1 locked by anotherLock2", reason)
				require.Equal(t, lockID, res.LockID)
				require.False(t, res.IsTaken)
			},
		},
		{
			name:   "conflict wall-e",
			holder: lockcluster.WalleCMS,
			prepare: func(m *mlockclientmocks.MockLocker, d *clusterdiscoverymocks.MockOtherLegsDiscovery) {
				holder := string(lockcluster.WalleCMS)
				d.EXPECT().FindInShardOrSubcidByFQDN(gomock.Any(), fqdn).Return(
					clusterdiscovery.Neighbourhood{
						Others: []metadb.Host{{FQDN: "f1"}, {FQDN: "f2"}},
						ID:     clusterID,
					}, nil)
				m.EXPECT().CreateLock(gomock.Any(), lockID, holder, []string{"f1", "f2", fqdn}, gomock.Any()).Return(nil)
				m.EXPECT().GetLockStatus(gomock.Any(), lockID).Return(mlockclient.LockStatus{
					Conflicts: []mlockclient.Conflict{
						{
							Object:  fqdn,
							LockIDs: []string{string(lockcluster.InstanceCMS)},
						},
					},
					Holder: holder,
					ID:     lockID,
				}, nil)
			},
			validate: func(t *testing.T, res *lockcluster.State, err error) {
				require.Error(t, err)
				require.True(t, xerrors.Is(err, lockcluster.NotAcquiredConflicts))
				reason, err := lockcluster.ErrorReason(err)
				require.True(t, xerrors.Is(err, lockcluster.NotAcquiredConflicts))
				require.Equal(t, "could not acquire lock \"cms-taskID-clusterID\""+
					" because of conflicts: fqdn locked by mdb-cms-instance", reason)
				require.Equal(t, lockID, res.LockID)
				require.False(t, res.IsTaken)
			},
		},
		{
			name:   "reuse instance",
			holder: lockcluster.InstanceCMS,
			prepare: func(m *mlockclientmocks.MockLocker, d *clusterdiscoverymocks.MockOtherLegsDiscovery) {
				holder := string(lockcluster.InstanceCMS)
				d.EXPECT().FindInShardOrSubcidByFQDN(gomock.Any(), fqdn).Return(
					clusterdiscovery.Neighbourhood{
						Others: []metadb.Host{{FQDN: "f1"}, {FQDN: "f2"}},
						ID:     clusterID,
					}, nil)
				m.EXPECT().CreateLock(gomock.Any(), lockID, holder, []string{"f1", "f2", fqdn}, gomock.Any()).Return(nil)
				m.EXPECT().GetLockStatus(gomock.Any(), lockID).Return(mlockclient.LockStatus{
					Conflicts: []mlockclient.Conflict{
						{
							Object:  fqdn,
							LockIDs: []string{"anotherLock"},
						},
					},
					Holder: holder,
					ID:     lockID,
				}, nil)
				m.EXPECT().GetLockStatus(gomock.Any(), "anotherLock").Return(mlockclient.LockStatus{
					Acquired: true,
					Holder:   string(lockcluster.WalleCMS),
				}, nil)
				m.EXPECT().ReleaseLock(gomock.Any(), lockID).Return(nil)
			},
			validate: func(t *testing.T, res *lockcluster.State, err error) {
				require.NoError(t, err)
				require.Equal(t, "anotherLock", res.LockID)
				require.True(t, res.IsTaken)
			},
		},
		{
			name:   "can reuse but not acquired",
			holder: lockcluster.InstanceCMS,
			prepare: func(m *mlockclientmocks.MockLocker, d *clusterdiscoverymocks.MockOtherLegsDiscovery) {
				holder := string(lockcluster.InstanceCMS)
				d.EXPECT().FindInShardOrSubcidByFQDN(gomock.Any(), fqdn).Return(
					clusterdiscovery.Neighbourhood{
						Others: []metadb.Host{{FQDN: "f1"}, {FQDN: "f2"}},
						ID:     clusterID,
					}, nil)
				m.EXPECT().CreateLock(gomock.Any(), lockID, holder, []string{"f1", "f2", fqdn}, gomock.Any()).Return(nil)
				m.EXPECT().GetLockStatus(gomock.Any(), lockID).Return(mlockclient.LockStatus{
					Conflicts: []mlockclient.Conflict{
						{
							Object:  fqdn,
							LockIDs: []string{"anotherLock"},
						},
					},
					Holder: holder,
					ID:     lockID,
				}, nil)
				m.EXPECT().GetLockStatus(gomock.Any(), "anotherLock").Return(mlockclient.LockStatus{
					Acquired: false,
					Holder:   string(lockcluster.WalleCMS),
				}, nil)
			},
			validate: func(t *testing.T, res *lockcluster.State, err error) {
				require.Error(t, err)
				require.True(t, xerrors.Is(err, lockcluster.NotAcquiredConflicts))
				reason, err := lockcluster.ErrorReason(err)
				require.True(t, xerrors.Is(err, lockcluster.NotAcquiredConflicts))
				require.Equal(t, "could not acquire lock \"cms-taskID-clusterID\""+
					" because of conflicts: fqdn locked by anotherLock", reason)
				require.Equal(t, lockID, res.LockID)
				require.False(t, res.IsTaken)
			},
		},
		{
			name:   "reuse wall-e",
			holder: lockcluster.WalleCMS,
			prepare: func(m *mlockclientmocks.MockLocker, d *clusterdiscoverymocks.MockOtherLegsDiscovery) {
				holder := string(lockcluster.WalleCMS)
				d.EXPECT().FindInShardOrSubcidByFQDN(gomock.Any(), fqdn).Return(
					clusterdiscovery.Neighbourhood{
						Others: []metadb.Host{{FQDN: "f1"}, {FQDN: "f2"}},
						ID:     clusterID,
					}, nil)
				m.EXPECT().CreateLock(gomock.Any(), lockID, holder, []string{"f1", "f2", fqdn}, gomock.Any()).Return(nil)
				m.EXPECT().GetLockStatus(gomock.Any(), lockID).Return(mlockclient.LockStatus{
					Conflicts: []mlockclient.Conflict{
						{
							Object:  fqdn,
							LockIDs: []string{"anotherLock"},
						},
					},
					Holder: holder,
					ID:     lockID,
				}, nil)
			},
			validate: func(t *testing.T, res *lockcluster.State, err error) {
				require.Error(t, err)
				require.True(t, xerrors.Is(err, lockcluster.NotAcquiredConflicts))
				reason, err := lockcluster.ErrorReason(err)
				require.True(t, xerrors.Is(err, lockcluster.NotAcquiredConflicts))
				require.Equal(t, "could not acquire lock \"cms-taskID-clusterID\""+
					" because of conflicts: fqdn locked by anotherLock", reason)
				require.Equal(t, lockID, res.LockID)
				require.False(t, res.IsTaken)
			},
		},
		{
			name: "unmanaged",
			prepare: func(m *mlockclientmocks.MockLocker, d *clusterdiscoverymocks.MockOtherLegsDiscovery) {
				d.EXPECT().FindInShardOrSubcidByFQDN(gomock.Any(), fqdn).Return(
					clusterdiscovery.Neighbourhood{}, semerr.NotFound("not found"))
			},
			validate: func(t *testing.T, res *lockcluster.State, err error) {
				require.Error(t, err)
				require.True(t, xerrors.Is(err, lockcluster.UnmanagedHost))
				require.Nil(t, res)
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			m := mlockclientmocks.NewMockLocker(ctrl)
			d := clusterdiscoverymocks.NewMockOtherLegsDiscovery(ctrl)
			tc.prepare(m, d)
			locker := mlock.NewLocker(m, d)
			id, err := locker.LockCluster(ctx, fqdn, taskID, tc.holder)
			tc.validate(t, id, err)
		})

	}
}
