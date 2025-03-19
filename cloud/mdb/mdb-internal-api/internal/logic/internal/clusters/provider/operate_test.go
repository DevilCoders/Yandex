package provider

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	sessionsmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	metadbmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type clusterFixture struct {
	Ctrl     *gomock.Controller
	Sessions *sessionsmock.MockSessions
	MetaDB   *metadbmock.MockBackend
	Cluster  *Clusters
	Operator *Operator
}

func newMocks(t *testing.T) (context.Context, clusterFixture) {
	ctrl := gomock.NewController(t)
	sess := sessionsmock.NewMockSessions(ctrl)
	mdb := metadbmock.NewMockBackend(ctrl)
	c := &Clusters{metaDB: mdb, sessions: sess}
	o := &Operator{metadb: mdb, sessions: sess}
	return context.Background(),
		clusterFixture{
			Ctrl:     ctrl,
			Sessions: sess,
			MetaDB:   mdb,
			Cluster:  c,
			Operator: o,
		}
}

func TestClusters_operate(t *testing.T) {
	resolver := sessions.ResolveByCluster("foo", models.PermMDBAllCreate)

	t.Run("Success", func(t *testing.T) {
		ctx, f := newMocks(t)

		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{}, sessions.OriginalRequest{}, nil)
		commitCall := f.Sessions.EXPECT().Commit(ctx)
		f.Sessions.EXPECT().Rollback(ctx).After(commitCall)

		expectedOp := operations.Operation{OperationID: testutil.NewUUIDStr(t)}
		do := func(ctx context.Context, session sessions.Session) (operations.Operation, error) {
			return expectedOp, nil
		}

		op, err := f.Operator.operate(ctx, resolver, do)
		require.NoError(t, err)
		require.Equal(t, expectedOp, op)
	})

	t.Run("BeginError", func(t *testing.T) {
		ctx, f := newMocks(t)

		expectedErr := xerrors.New("error")
		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{}, sessions.OriginalRequest{}, expectedErr)

		op, err := f.Operator.operate(ctx, resolver, nil)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Equal(t, operations.Operation{}, op)
	})

	t.Run("OperationExists", func(t *testing.T) {
		ctx, f := newMocks(t)

		expectedOp := operations.Operation{OperationID: testutil.NewUUIDStr(t)}
		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{}, sessions.OriginalRequest{Op: expectedOp, Exists: true}, nil)
		f.Sessions.EXPECT().Rollback(ctx)

		op, err := f.Operator.operate(ctx, resolver, nil)
		require.NoError(t, err)
		require.Equal(t, expectedOp, op)
	})

	t.Run("DoError", func(t *testing.T) {
		ctx, f := newMocks(t)

		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{}, sessions.OriginalRequest{}, nil)
		f.Sessions.EXPECT().Rollback(ctx)

		expectedErr := xerrors.New("error")
		do := func(ctx context.Context, session sessions.Session) (operations.Operation, error) {
			return operations.Operation{}, expectedErr
		}

		op, err := f.Operator.operate(ctx, resolver, do)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Equal(t, operations.Operation{}, op)
	})

	t.Run("CommitError", func(t *testing.T) {
		ctx, f := newMocks(t)

		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{}, sessions.OriginalRequest{}, nil)
		expectedErr := xerrors.New("error")
		commitCall := f.Sessions.EXPECT().Commit(ctx).Return(expectedErr)
		f.Sessions.EXPECT().Rollback(ctx).After(commitCall)

		unexpectedOp := operations.Operation{OperationID: testutil.NewUUIDStr(t)}
		do := func(ctx context.Context, session sessions.Session) (operations.Operation, error) {
			return unexpectedOp, nil
		}

		op, err := f.Operator.operate(ctx, resolver, do)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Equal(t, operations.Operation{}, op)
		require.NotEqual(t, unexpectedOp, op)
	})
}

func TestClusters_modify(t *testing.T) {
	rid := requestid.New()
	fc := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud_ext_id_1",
		FolderID:    2,
		FolderExtID: "folder_ext_id_2",
	}
	cid := testutil.NewUUIDStr(t)
	typ := clmodels.TypeClickHouse
	resolver := sessions.ResolveByClusterDependency(cid, models.PermMDBAllCreate)

	t.Run("Success", func(t *testing.T) {
		ctx, f := newMocks(t)
		ctx = requestid.WithRequestID(ctx, rid)
		res := quota.Resources{CPU: 1}

		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{FolderCoords: fc, Quota: quota.NewConsumption(res, quota.Resources{})}, sessions.OriginalRequest{}, nil)
		commitCall := f.Sessions.EXPECT().Commit(ctx)
		f.Sessions.EXPECT().Rollback(ctx).After(commitCall)

		cluster := metadb.Cluster{Cluster: clmodels.Cluster{Type: typ, Revision: 1}}
		f.MetaDB.EXPECT().LockCluster(ctx, cid, rid).Return(cluster, nil)
		f.MetaDB.EXPECT().UpdateCloudUsedQuota(ctx, fc.CloudID, metadb.Resources(res), rid)
		f.MetaDB.EXPECT().CompleteClusterChange(ctx, cid, cluster.Revision)

		expectedOp := operations.Operation{OperationID: testutil.NewUUIDStr(t)}
		do := func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			session.Quota.RequestChange(res)
			return expectedOp, nil
		}

		op, err := f.Operator.modify(ctx, cid, typ, resolver, true, do)
		require.NoError(t, err)
		require.Equal(t, expectedOp, op)
	})

	t.Run("NoQuotaChange", func(t *testing.T) {
		ctx, f := newMocks(t)
		ctx = requestid.WithRequestID(ctx, rid)
		resolver := sessions.ResolveByClusterDependency(cid, models.PermMDBAllCreate)

		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{FolderCoords: fc, Quota: quota.NewConsumption(quota.Resources{}, quota.Resources{})}, sessions.OriginalRequest{}, nil)
		commitCall := f.Sessions.EXPECT().Commit(ctx)
		f.Sessions.EXPECT().Rollback(ctx).After(commitCall)

		cluster := metadb.Cluster{Cluster: clmodels.Cluster{Type: typ, Revision: 1}}
		f.MetaDB.EXPECT().LockCluster(ctx, cid, rid).Return(cluster, nil)
		f.MetaDB.EXPECT().CompleteClusterChange(ctx, cid, cluster.Revision)

		expectedOp := operations.Operation{OperationID: testutil.NewUUIDStr(t)}
		do := func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			return expectedOp, nil
		}

		op, err := f.Operator.modify(ctx, cid, typ, resolver, true, do)
		require.NoError(t, err)
		require.Equal(t, expectedOp, op)
	})

	t.Run("LockClusterError", func(t *testing.T) {
		ctx, f := newMocks(t)
		ctx = requestid.WithRequestID(ctx, rid)
		resolver := sessions.ResolveByClusterDependency(cid, models.PermMDBAllCreate)

		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{Quota: quota.NewConsumption(quota.Resources{}, quota.Resources{})}, sessions.OriginalRequest{}, nil)
		f.Sessions.EXPECT().Rollback(ctx)

		cluster := metadb.Cluster{Cluster: clmodels.Cluster{Type: typ, Revision: 1}}
		f.MetaDB.EXPECT().LockCluster(ctx, cid, rid).Return(cluster, nil)
		expectedErr := xerrors.New("error")
		f.MetaDB.EXPECT().CompleteClusterChange(ctx, cid, cluster.Revision).Return(expectedErr)

		unexpectedOp := operations.Operation{OperationID: testutil.NewUUIDStr(t)}
		do := func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			return unexpectedOp, nil
		}

		op, err := f.Operator.modify(ctx, cid, typ, resolver, true, do)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Equal(t, operations.Operation{}, op)
		require.NotEqual(t, unexpectedOp, op)
	})

	t.Run("CompleteClusterChangeError", func(t *testing.T) {
		ctx, f := newMocks(t)
		ctx = requestid.WithRequestID(ctx, rid)
		resolver := sessions.ResolveByClusterDependency(cid, models.PermMDBAllCreate)

		f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any()).
			Return(ctx, sessions.Session{}, sessions.OriginalRequest{}, nil)
		f.Sessions.EXPECT().Rollback(ctx)

		expectedErr := xerrors.New("error")
		f.MetaDB.EXPECT().LockCluster(ctx, cid, rid).Return(metadb.Cluster{}, expectedErr)

		op, err := f.Operator.modify(ctx, cid, typ, resolver, true, nil)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Equal(t, operations.Operation{}, op)
	})
}

type ResolverMatcher struct {
	Resolver sessions.SessionResolver
}

func (r *ResolverMatcher) Matches(x interface{}) bool {
	switch a := x.(type) {
	case sessions.SessionResolver:
		return r.Resolver.Target == a.Target
	}
	return false
}

func (r *ResolverMatcher) String() string {
	return fmt.Sprintf("is equal to %v", r.Resolver.Target)
}

func TestReadResolvers(t *testing.T) {
	testCases := []struct {
		name             string
		method           func(context.Context, clusterFixture) error
		expectedResolver sessions.SessionResolver
	}{
		{
			name: "Check ReadOnFolder resolver",
			method: func(ctx context.Context, f clusterFixture) (err error) {
				err = f.Operator.ReadOnFolder(ctx, "folder_ext_id_1", nil)
				return
			},
			expectedResolver: sessions.ResolveByFolder("folder_ext_id_1", models.PermMDBAllRead),
		},
		{
			name: "Check ReadCluster resolver",
			method: func(ctx context.Context, f clusterFixture) (err error) {
				err = f.Operator.ReadCluster(ctx, "cid_1", nil)
				return
			},
			expectedResolver: sessions.ResolveByCluster("cid_1", models.PermMDBAllRead),
		},
		{
			name: "Check ReadOnCluster resolver",
			method: func(ctx context.Context, f clusterFixture) (err error) {
				err = f.Operator.ReadOnCluster(ctx, "cid_2", clmodels.TypeUnknown, nil)
				return
			},
			expectedResolver: sessions.ResolveByClusterDependency("cid_2", models.PermMDBAllRead),
		},
	}
	ctx, f := newMocks(t)
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			f.Sessions.EXPECT().Begin(ctx, &ResolverMatcher{tc.expectedResolver}).
				Return(ctx, sessions.Session{}, xerrors.New("error to return"))

			err := tc.method(ctx, f)
			require.Errorf(t, err, "error to return")
		})
	}
}

func TestModifyResolvers(t *testing.T) {
	testCases := []struct {
		name             string
		method           func(context.Context, clusterFixture) error
		expectedResolver sessions.SessionResolver
	}{
		{
			name: "Check Create resolver",
			method: func(ctx context.Context, f clusterFixture) (err error) {
				_, err = f.Operator.Create(ctx, "folder_ext_id_1", nil)
				return
			},
			expectedResolver: sessions.ResolveByFolder("folder_ext_id_1", models.PermMDBAllCreate),
		},
		{
			name: "Check Delete resolver",
			method: func(ctx context.Context, f clusterFixture) (err error) {
				_, err = f.Operator.Delete(ctx, "cid_1", clmodels.TypeUnknown, nil)
				return
			},
			expectedResolver: sessions.ResolveByCluster("cid_1", models.PermMDBAllDelete),
		},
		{
			name: "Check CreateOnCluster resolver",
			method: func(ctx context.Context, f clusterFixture) (err error) {
				_, err = f.Operator.CreateOnCluster(ctx, "cid_2", clmodels.TypeUnknown, nil)
				return
			},
			expectedResolver: sessions.ResolveByClusterDependency("cid_2", models.PermMDBAllCreate),
		},
		{
			name: "Check ModifyOnCluster resolver",
			method: func(ctx context.Context, f clusterFixture) (err error) {
				_, err = f.Operator.ModifyOnCluster(ctx, "cid_3", clmodels.TypeUnknown, nil)
				return
			},
			expectedResolver: sessions.ResolveByClusterDependency("cid_3", models.PermMDBAllModify),
		},
		{
			name: "Check DeleteOnCluster resolver",
			method: func(ctx context.Context, f clusterFixture) (err error) {
				_, err = f.Operator.DeleteOnCluster(ctx, "cid_4", clmodels.TypeUnknown, nil)
				return
			},
			expectedResolver: sessions.ResolveByClusterDependency("cid_4", models.PermMDBAllDelete),
		},
	}
	ctx, f := newMocks(t)
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			f.Sessions.EXPECT().BeginWithIdempotence(ctx, &ResolverMatcher{tc.expectedResolver}).
				Return(ctx, sessions.Session{}, sessions.OriginalRequest{}, xerrors.New("error to return"))

			err := tc.method(ctx, f)
			require.Errorf(t, err, "error to return")
		})
	}
}
