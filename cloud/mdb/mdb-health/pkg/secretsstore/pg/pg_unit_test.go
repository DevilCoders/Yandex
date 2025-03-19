package pg_test

import (
	"context"
	"database/sql"
	"testing"

	"github.com/DATA-DOG/go-sqlmock"
	"github.com/gofrs/uuid"
	"github.com/jmoiron/sqlx"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

func TestPGSecretStore(t *testing.T) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))

	db, mock, err := sqlmock.New()
	require.NoError(t, err)
	require.NotNil(t, db)
	require.NotNil(t, mock)
	defer func() { _ = db.Close() }()

	cluster, err := sqlutil.NewCluster(
		[]sqlutil.Node{
			sqlutil.NewNode("sqlmock", sqlx.NewDb(db, "sqlmock")),
		},
		// Disable background node checking because its asynchronous and we can't really write correct expectations
		func(ctx context.Context, db *sql.DB) (bool, error) {
			return true, nil
		},
		sqlutil.WithTracer(tracers.Log(logger)),
	)
	require.NoError(t, err)
	require.NotNil(t, cluster)

	_, err = cluster.WaitForPrimary(ctx)
	require.NoError(t, err)

	ss := pg.NewWithCluster(
		cluster,
		logger,
	)
	require.NotNil(t, ss)

	// Test 'unknown' cid
	cid := uuid.Must(uuid.NewV4()).String()

	mock.ExpectQuery("SELECT public_key FROM dbaas.clusters WHERE cid = ?").
		WithArgs(cid).
		WillReturnRows(sqlmock.NewRows([]string{"public_key"}))

	secret, err := ss.LoadClusterSecret(ctx, cid)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, secretsstore.ErrSecretNotFound))
	require.Nil(t, secret)

	// Test 'known' cid
	cid = uuid.Must(uuid.NewV4()).String()
	secret = uuid.Must(uuid.NewV4()).Bytes()

	mock.ExpectQuery("SELECT public_key FROM dbaas.clusters WHERE cid = ?").
		WithArgs(cid).
		WillReturnRows(
			sqlmock.NewRows([]string{"public_key"}).
				AddRow(secret),
		)

	loadedSecret, err := ss.LoadClusterSecret(ctx, cid)
	require.NoError(t, err)
	require.Equal(t, secret, loadedSecret)
}
