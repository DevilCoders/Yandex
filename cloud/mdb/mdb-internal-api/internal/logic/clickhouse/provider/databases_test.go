package provider

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type testInput struct {
	Name string
	Run  func(ctx context.Context, f databasesFixture)
}

type testInputWithError struct {
	Name string
	Run  func(ctx context.Context, f databasesFixture, expectedErr error)
}

func TestClickHouse_Database(t *testing.T) {
	t.Run("Found", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		f.ExpectSubClusterByRole(
			ctx,
			clusters.SubCluster{ClusterID: f.ClusterID, SubClusterID: f.SubClusterID},
			&chpillars.SubClusterCH{
				Data: chpillars.SubClusterCHData{
					ClickHouse: chpillars.SubClusterCHServer{
						DBs: []string{f.DBName},
					},
				},
			},
		)

		db, err := f.ClickHouse.database(ctx, f.Reader, f.ClusterID, f.DBName)
		require.NoError(t, err)
		require.Equal(t, chmodels.Database{ClusterID: f.ClusterID, Name: f.DBName}, db)
	})

	t.Run("NotFound", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		f.ExpectSubClusterByRole(ctx, clusters.SubCluster{ClusterID: f.ClusterID, SubClusterID: f.SubClusterID}, &chpillars.SubClusterCH{})

		db, err := f.ClickHouse.database(ctx, f.Reader, f.ClusterID, f.DBName)
		require.Error(t, err)
		require.True(t, semerr.IsNotFound(err))
		require.Equal(t, chmodels.Database{}, db)
	})

	t.Run("SubClusterByRoleError", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		expectedErr := xerrors.New("error")
		f.ExpectSubClusterByRoleError(ctx, f.ClusterID, expectedErr)

		db, err := f.ClickHouse.database(ctx, f.Reader, f.ClusterID, f.DBName)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Equal(t, chmodels.Database{}, db)
	})
}

func TestClickHouse_Databases(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		secondDBName := testutil.NewUUIDStr(t)
		f.ExpectSubClusterByRole(
			ctx,
			clusters.SubCluster{ClusterID: f.ClusterID, SubClusterID: f.SubClusterID},
			&chpillars.SubClusterCH{
				Data: chpillars.SubClusterCHData{
					ClickHouse: chpillars.SubClusterCHServer{
						DBs: []string{
							f.DBName,
							secondDBName,
						},
					},
				},
			},
		)

		dbs, err := f.ClickHouse.databases(ctx, f.Reader, f.ClusterID)
		require.NoError(t, err)
		expected := []chmodels.Database{
			{ClusterID: f.ClusterID, Name: f.DBName},
			{ClusterID: f.ClusterID, Name: secondDBName},
		}
		require.Len(t, dbs, len(expected))
		require.Contains(t, dbs, expected[0])
		require.Contains(t, dbs, expected[1])
	})

	t.Run("SubClusterByRoleError", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		expectedErr := xerrors.New("error")
		f.ExpectSubClusterByRoleError(ctx, f.ClusterID, expectedErr)

		dbs, err := f.ClickHouse.databases(ctx, f.Reader, f.ClusterID)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Len(t, dbs, 0)
	})
}

func TestClickHouse_CreateDatabase(t *testing.T) {
	t.Run("InvalidName", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)
		_, err := f.ClickHouse.CreateDatabase(ctx, f.ClusterID, chmodels.DatabaseSpec{Name: "default"})
		require.Error(t, err)
		require.True(t, semerr.IsInvalidInput(err))
	})
}

func TestClickHouse_createDatabase(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		f.ExpectSubClusterByRole(ctx, clusters.SubCluster{ClusterID: f.ClusterID, SubClusterID: f.SubClusterID}, &chpillars.SubClusterCH{})
		f.ExpectUpdateSubClusterPillar(
			ctx,
			f.ClusterID,
			f.SubClusterID,
			1,
			func(pillar interface{}) {
				p := pillar.(*chpillars.SubClusterCH)
				require.Contains(t, p.Data.ClickHouse.DBs, f.DBName)
			},
		)

		expectedOp := operations.Operation{ClusterID: f.ClusterID, TargetID: f.ClusterID}
		session := sessions.Session{}
		f.ExpectCreateTask(ctx, session, expectedOp)
		f.ExpectEvent(ctx)

		op, err := f.ClickHouse.createDatabase(ctx, f.Reader, f.Modifier, session, f.LoadedCluster(), f.ClusterID, chmodels.DatabaseSpec{Name: f.DBName})
		require.NoError(t, err)
		require.Equal(t, expectedOp, op)
	})

	t.Run("AlreadyExists", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		f.ExpectSubClusterByRole(
			ctx,
			clusters.SubCluster{ClusterID: f.ClusterID, SubClusterID: f.SubClusterID},
			&chpillars.SubClusterCH{
				Data: chpillars.SubClusterCHData{
					ClickHouse: chpillars.SubClusterCHServer{
						DBs: []string{f.DBName},
					},
				},
			},
		)

		op, err := f.ClickHouse.createDatabase(ctx, f.Reader, f.Modifier, sessions.Session{}, f.LoadedCluster(), f.ClusterID, chmodels.DatabaseSpec{Name: f.DBName})
		require.Error(t, err)
		require.True(t, semerr.IsAlreadyExists(err))
		require.Equal(t, operations.Operation{}, op)
	})

	t.Run("SubClusterByRoleError", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		expectedErr := xerrors.New("error")
		f.ExpectSubClusterByRoleError(ctx, f.ClusterID, expectedErr)

		op, err := f.ClickHouse.createDatabase(ctx, f.Reader, f.Modifier, sessions.Session{}, f.LoadedCluster(), f.ClusterID, chmodels.DatabaseSpec{Name: f.DBName})
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Equal(t, operations.Operation{}, op)
	})

	t.Run("UpdateSubClusterPillarError", func(t *testing.T) {
		ctx, f := newDatabasesFixture(t)

		f.ExpectSubClusterByRole(ctx, clusters.SubCluster{ClusterID: f.ClusterID, SubClusterID: f.SubClusterID}, &chpillars.SubClusterCH{})
		expectedErr := xerrors.New("error")
		f.ExpectUpdateSubClusterPillarError(ctx, f.ClusterID, f.SubClusterID, 1, expectedErr)

		op, err := f.ClickHouse.createDatabase(ctx, f.Reader, f.Modifier, sessions.Session{}, f.LoadedCluster(), f.ClusterID, chmodels.DatabaseSpec{Name: f.DBName})
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
		require.Equal(t, operations.Operation{}, op)
	})
}

type databasesFixture struct {
	clickHouseFixture

	ClusterID    string
	ClusterName  string
	SubClusterID string
	DBName       string
}

func newDatabasesFixture(t *testing.T) (context.Context, databasesFixture) {
	ctx, f := newClickHouseFixture(t)
	return ctx, databasesFixture{
		clickHouseFixture: f,
		ClusterID:         testutil.NewUUIDStr(t),
		ClusterName:       testutil.NewUUIDStr(t),
		SubClusterID:      testutil.NewUUIDStr(t),
		DBName:            testutil.NewUUIDStr(t),
	}
}

func (f databasesFixture) LoadedCluster() clusterslogic.Cluster {
	return clusterslogic.Cluster{
		Cluster: clusters.Cluster{
			ClusterID: f.ClusterID,
			Name:      f.ClusterName,
			Revision:  1,
		},
	}
}
