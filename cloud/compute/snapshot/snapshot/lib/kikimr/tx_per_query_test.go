package kikimr

import (
	"database/sql"
	"testing"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"
	"go.uber.org/zap/zaptest"
	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

func TestTransactionPerQueryExecContext(t *testing.T) {
	r := require.New(t)

	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t))
	config, err := config.GetConfig()
	r.NoError(err)
	storageI, err := NewKikimr(ctx, &config, misc.TableOpCreate)
	r.NoError(err)
	storage := storageI.(*kikimrstorage)
	q := sqlTxPerQuery{db: storage.db}

	id := uuid.Must(uuid.NewV4()).String()
	_, err = q.ExecContext(ctx, `#PREPARE
DECLARE $id AS Utf8;

UPSERT INTO $snapshots_locks (id) VALUES($id)
`, sql.Named("id", id))
	r.NoError(err)

	row := q.QueryRowContext(ctx, `#PREPARE
DECLARE $id AS Utf8;

SELECT id FROM $snapshots_locks
WHERE id=$id
`, sql.Named("id", id))
	r.NoError(row.Err())
	var idRes *string
	err = row.Scan(&idRes)
	r.NoError(err)
	if idRes == nil {
		r.Fail("idRes is nil")
	} else {
		r.EqualValues(id, *idRes)
	}

	_, err = q.ExecContext(ctx, `#PREPARE
DECLARE $id AS Utf8;

DELETE FROM $snapshots_locks WHERE id=$id
`, sql.Named("id", id))
	r.NoError(err)
}

func TestTransactionPerQueryQueryContext(t *testing.T) {
	r := require.New(t)

	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t))

	config, err := config.GetConfig()
	r.NoError(err)
	storageI, err := NewKikimr(ctx, &config, misc.TableOpCreate)
	r.NoError(err)
	storage := storageI.(*kikimrstorage)
	q := sqlTxPerQuery{db: storage.db}

	res, err := q.QueryContext(ctx, `#PREPARE
DECLARE $a AS Int64;
DECLARE $b AS Int64;

SELECT ($a + $b) AS sum
`, sql.Named("a", int64(1)), sql.Named("b", int64(2)))
	r.NoError(err)

	r.True(res.Next())

	var resSum int
	err = res.Scan(&resSum)
	r.NoError(err)
	r.Equal(resSum, 3)
}

func TestTransactionPerQueryQueryRowContext(t *testing.T) {
	r := require.New(t)

	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t))
	config, err := config.GetConfig()
	r.NoError(err)
	storageI, err := NewKikimr(ctx, &config, misc.TableOpCreate)
	r.NoError(err)
	storage := storageI.(*kikimrstorage)
	q := sqlTxPerQuery{db: storage.db}

	row := q.QueryRowContext(ctx, `#PREPARE
DECLARE $a AS Int64;
DECLARE $b AS Int64;

SELECT ($a + $b) AS sum
`, sql.Named("a", int64(1)), sql.Named("b", int64(2)))
	r.NoError(row.Err())

	var resSum int
	err = row.Scan(&resSum)
	r.NoError(err)
	r.Equal(resSum, 3)
}
