package pg

import (
	"context"
	"errors"
	"fmt"

	"github.com/jackc/pgconn"
	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/mlockdb"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var (
	createLockStmt = sqlutil.Stmt{
		Name:  "CreateLock",
		Query: "SELECT code.create_lock(i_lock_ext_id => :lock_id, i_holder => :holder, i_reason => :reason, i_objects => :objects)",
	}

	releaseLockStmt = sqlutil.Stmt{
		Name:  "ReleaseLock",
		Query: "SELECT code.release_lock(i_lock_ext_id => :lock_id)",
	}

	getLockStatusStmt = sqlutil.Stmt{
		Name:  "GetLockStatus",
		Query: "SELECT * FROM code.get_lock_status(i_lock_ext_id => :lock_id)",
	}

	getLocksStmt = sqlutil.Stmt{
		Name:  "GetLocks",
		Query: "SELECT * FROM code.get_locks(i_holder => :holder, i_limit => :limit, i_offset => :offset)",
	}
)

func wrapError(err error) error {
	if err != nil {
		var pgError *pgconn.PgError
		if errors.As(err, &pgError) {
			switch pgError.Code {
			case "MLD01":
				return fmt.Errorf("%w: %s", mlockdb.ErrConflict, pgError.Message)
			case "MLD02":
				return fmt.Errorf("%w: %s", mlockdb.ErrNotFound, pgError.Message)
			}
		}
	}
	return err
}

// DefaultConfig returns default configuration for PostgreSQL mlockdb implementation
func DefaultConfig() pgutil.Config {
	config := pgutil.DefaultConfig()
	config.DB = "mlockdb"
	config.User = "mlock"
	config.SSLMode = pgutil.VerifyFullSSLMode
	return config
}

type db struct {
	logger log.Logger

	cluster *sqlutil.Cluster
}

var _ mlockdb.MlockDB = &db{}

// New is a constructor for PostgreSQL mlockdb implementation
func New(cfg pgutil.Config, logger log.Logger) (mlockdb.MlockDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(logger)))
	if err != nil {
		return nil, err
	}

	return &db{
		logger:  logger,
		cluster: cluster,
	}, nil
}

func (db *db) Close() error {
	return db.cluster.Close()
}

func (db *db) IsReady(ctx context.Context) error {
	node := db.cluster.Alive()
	if node == nil {
		return mlockdb.ErrNotAvailable
	}

	return nil
}

func (db *db) Begin(ctx context.Context, ns sqlutil.NodeStateCriteria) (context.Context, error) {
	binding, err := sqlutil.Begin(ctx, db.cluster, ns, nil)
	if err != nil {
		if semerr.IsUnavailable(err) {
			return ctx, mlockdb.ErrNotAvailable
		}

		return ctx, err
	}

	return sqlutil.WithTxBinding(ctx, binding), nil
}

func (db *db) Commit(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Commit(ctx)
}

func (db *db) Rollback(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Rollback(ctx)
}

func (db *db) CreateLock(ctx context.Context, lock models.Lock) error {
	objects := make([]string, len(lock.Objects))

	for index, object := range lock.Objects {
		objects[index] = string(object)
	}

	dbObjects := &pgtype.TextArray{}

	err := dbObjects.Set(objects)

	if err != nil {
		return fmt.Errorf("unable to format objects into TextArray: %w", err)
	}

	_, err = sqlutil.QueryContext(
		ctx,
		db.cluster.PrimaryChooser(),
		createLockStmt,
		map[string]interface{}{
			"lock_id": lock.ID,
			"holder":  lock.Holder,
			"reason":  lock.Reason,
			"objects": dbObjects,
		},
		sqlutil.NopParser,
		db.logger,
	)

	return wrapError(err)
}

func (db *db) ReleaseLock(ctx context.Context, lockID models.LockID) error {
	_, err := sqlutil.QueryContext(
		ctx,
		db.cluster.PrimaryChooser(),
		releaseLockStmt,
		map[string]interface{}{
			"lock_id": lockID,
		},
		sqlutil.NopParser,
		db.logger,
	)

	return wrapError(err)
}

func (db *db) GetLocks(ctx context.Context, holder models.LockHolder, limit int64, offset int64) ([]models.Lock, bool, error) {
	var locks []models.Lock
	var hasMore bool

	parser := func(rows *sqlx.Rows) error {
		if int64(len(locks)) < limit {
			var lock Lock
			if err := rows.StructScan(&lock); err != nil {
				return err
			}

			locks = append(locks, lock.ToInternal())
		} else {
			hasMore = true
		}

		return nil
	}

	params := map[string]interface{}{
		"limit":  limit + 1, // We pass limit+1 here to check if we have more data for next page
		"offset": offset,
	}

	if holder != "" {
		params["holder"] = holder
	} else {
		params["holder"] = nil
	}

	_, err := sqlutil.QueryContext(
		ctx,
		db.cluster.AliveChooser(),
		getLocksStmt,
		params,
		parser,
		db.logger,
	)

	return locks, hasMore, err
}

func (db *db) GetLockStatus(ctx context.Context, lockID models.LockID) (models.LockStatus, error) {
	var lock models.LockStatus

	parser := func(rows *sqlx.Rows) error {
		var dbLock LockStatus
		err := rows.StructScan(&dbLock)
		if err != nil {
			return err
		}

		lock, err = dbLock.ToInternal()

		return err
	}

	_, err := sqlutil.QueryContext(
		ctx,
		db.cluster.PrimaryChooser(),
		getLockStatusStmt,
		map[string]interface{}{
			"lock_id": lockID,
		},
		parser,
		db.logger,
	)

	return lock, wrapError(err)
}
