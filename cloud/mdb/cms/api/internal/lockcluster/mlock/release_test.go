package mlock_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster/mlock"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/mocks"
)

func TestReleaseCluster(t *testing.T) {
	ctx := context.Background()
	lockID := "cms-taskID-clusterID"
	tcs := []struct {
		name     string
		prepare  func(m *mocks.MockLocker)
		validate func(t *testing.T, err error)
	}{
		{
			name: "simple release",
			prepare: func(m *mocks.MockLocker) {
				m.EXPECT().ReleaseLock(gomock.Any(), lockID).Return(nil)
			},
			validate: func(t *testing.T, err error) {
				require.NoError(t, err)
			},
		},
		{
			name: "unexisted lock",
			prepare: func(m *mocks.MockLocker) {
				m.EXPECT().ReleaseLock(gomock.Any(), lockID).Return(semerr.NotFound("not found"))
			},
			validate: func(t *testing.T, err error) {
				require.NoError(t, err)
			},
		},
		{
			name: "release error",
			prepare: func(m *mocks.MockLocker) {
				m.EXPECT().ReleaseLock(gomock.Any(), lockID).Return(semerr.Unavailable("some error"))
			},
			validate: func(t *testing.T, err error) {
				require.Error(t, err)
				require.True(t, semerr.IsUnavailable(err))
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			m := mocks.NewMockLocker(ctrl)
			tc.prepare(m)
			locker := mlock.NewLocker(m, nil)
			err := locker.ReleaseCluster(ctx, &lockcluster.State{LockID: lockID})
			tc.validate(t, err)
		})

	}
}
