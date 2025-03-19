package storage

import (
	"context"
	"database/sql"
	"time"

	"github.com/jackc/pgconn"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/serialization"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/storage/types"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
	"a.yandex-team.ru/transfer_manager/go/pkg/cleanup"
	"a.yandex-team.ru/transfer_manager/go/pkg/dbaas"
	"a.yandex-team.ru/transfer_manager/go/pkg/logging"
	"a.yandex-team.ru/transfer_manager/go/pkg/server"
)

const (
	errConnectionWithIDNotFound        = "connection with id '%s' not found"
	errConnectionWithNameAlreadyExists = "connection with name '%s' already exists (or was previously deleted) in folder '%s'"
	errUnableToExecuteQuery            = "unable to execute query: %w"
	errUnableToGetAffectedRowsCount    = "unable to get affected rows count: %w"
	errUnableToReadRow                 = "unable to read row: %w"
	errUnableToScanRow                 = "unable to scan row: %w"
	errUnableToSerializeConnection     = "unable to serialize connection: %w"
	errUnableToDeserializeConnection   = "unable to deserialize connection: %w"
)

type TxFunc func(ctx context.Context) error

type Storage interface {
	ExecTx(ctx context.Context, txFunc TxFunc) error
	Get(ctx context.Context, connectionID string) (*types.Connection, error)
	List(ctx context.Context, folderID string) ([]*types.Connection, error)
	Create(ctx context.Context, connection *types.Connection) (*types.ConnectionOperation, error)
	Update(ctx context.Context, connection *types.Connection, prev *types.Connection) (*types.ConnectionOperation, error)
	Delete(ctx context.Context, connectionID string) (*types.ConnectionOperation, error)
}

type pgStorage struct {
	pg            *dbaas.PgHA
	serializer    serialization.Serializer
	loggerFactory logging.LoggerFactory
}

func NewPgStorage(pg *dbaas.PgHA, serializer serialization.Serializer, loggerFactory logging.LoggerFactory) Storage {
	return &pgStorage{
		pg:            pg,
		serializer:    serializer,
		loggerFactory: loggerFactory,
	}
}

func (s *pgStorage) ExecTx(ctx context.Context, txFunc TxFunc) error {
	logger := s.loggerFactory(ctx)

	db, err := s.pg.Sqlx(dbaas.MASTER)
	if err != nil {
		return xerrors.Errorf(server.ErrUnableCreateSQLX, err)
	}
	defer cleanup.Close(db, logger)

	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	tx, err := db.BeginTxx(ctx, &sql.TxOptions{Isolation: sql.LevelReadCommitted, ReadOnly: false})
	if err != nil {
		return xerrors.Errorf("unable to begin transaction: %w", err)
	}

	err = txFunc(withTx(ctx, tx))
	if err != nil {
		return err
	}

	err = tx.Commit()
	if err != nil {
		return xerrors.Errorf("unable to commit tx: %w", err)
	}

	return nil
}

func (s *pgStorage) Get(ctx context.Context, connectionID string) (*types.Connection, error) {
	logger := s.loggerFactory(ctx)

	var q interface {
		SelectContext(ctx context.Context, dest interface{}, query string, args ...interface{}) error
	}
	if tx := getTx(ctx); tx != nil {
		q = tx
	} else {
		db, err := s.pg.Sqlx(dbaas.ANY)
		if err != nil {
			return nil, xerrors.Errorf(server.ErrUnableCreateSQLX, err)
		}
		defer cleanup.Close(db, logger)
		q = db
	}

	query := `
SELECT id, folder_id, created_at, updated_at, deleted_at, name, description, labels, created_by, params
FROM connection
WHERE id = $1 AND deleted_at IS NULL
`
	var connections []*types.Connection
	err := q.SelectContext(ctx, &connections, query, connectionID)
	if err != nil {
		return nil, xerrors.Errorf(errUnableToExecuteQuery, err)
	}
	if len(connections) == 0 {
		return nil, xerrors.Errorf(errConnectionWithIDNotFound, connectionID)
	}

	connection := connections[0]
	err = s.serializer.Deserialize(ctx, connection.Params)
	if err != nil {
		return nil, xerrors.Errorf(errUnableToDeserializeConnection, err)
	}

	return connection, nil
}

func (s *pgStorage) List(ctx context.Context, folderID string) ([]*types.Connection, error) {
	logger := s.loggerFactory(ctx)

	db, err := s.pg.Sqlx(dbaas.ANY)
	if err != nil {
		return nil, xerrors.Errorf(server.ErrUnableCreateSQLX, err)
	}
	defer cleanup.Close(db, logger)

	var connections []*types.Connection
	query := `
SELECT id, folder_id, created_at, updated_at, deleted_at, name, description, labels, created_by, params
FROM connection
WHERE folder_id = $1 AND deleted_at IS NULL
`
	err = db.SelectContext(ctx, &connections, query, folderID)
	if err != nil {
		return nil, xerrors.Errorf("unable to select connection: %w", err)
	}

	for _, connection := range connections {
		err = s.serializer.Deserialize(ctx, connection.Params)
		if err != nil {
			return nil, xerrors.Errorf(errUnableToDeserializeConnection, err)
		}
	}

	return connections, nil
}

func (s *pgStorage) Create(ctx context.Context, connection *types.Connection) (*types.ConnectionOperation, error) {
	err := s.serializer.Serialize(ctx, connection.Params)
	if err != nil {
		return nil, xerrors.Errorf(errUnableToSerializeConnection, err)
	}

	tx := getTx(ctx)

	err = s.insertConnection(ctx, tx, connection)
	if err != nil {
		return nil, err
	}

	metadata := &connman.ConnectionOperationMetadata{Type: &connman.ConnectionOperationMetadata_Create{Create: &connman.CreateConnectionMetadata{}}}
	op, err := s.insertConnectionOperation(ctx, tx, connection.ID, metadata)
	if err != nil {
		return nil, xerrors.Errorf("unable to insert create operation: %w", err)
	}

	return op, nil
}

func (s *pgStorage) Update(ctx context.Context, connection *types.Connection, prev *types.Connection) (*types.ConnectionOperation, error) {
	err := s.serializer.Serialize(ctx, connection.Params)
	if err != nil {
		return nil, xerrors.Errorf(errUnableToSerializeConnection, err)
	}

	tx := getTx(ctx)

	err = s.updateConnection(ctx, connection, tx)
	if err != nil {
		return nil, err
	}

	metadata := &connman.ConnectionOperationMetadata{Type: &connman.ConnectionOperationMetadata_Update{Update: &connman.UpdateConnectionMetadata{Prev: prev.ToProto()}}}
	op, err := s.insertConnectionOperation(ctx, tx, connection.ID, metadata)
	if err != nil {
		return nil, xerrors.Errorf("unable to insert update operation: %w", err)
	}

	return op, nil
}

func (s *pgStorage) Delete(ctx context.Context, connectionID string) (*types.ConnectionOperation, error) {
	tx := getTx(ctx)

	query := `
UPDATE connection
SET deleted_at = NOW()
WHERE id = $1 AND deleted_at IS NULL
`
	result, err := tx.ExecContext(ctx, query, connectionID)
	if err != nil {
		return nil, xerrors.Errorf(errUnableToExecuteQuery, err)
	}
	rowsAffected, err := result.RowsAffected()
	if err != nil {
		return nil, xerrors.Errorf(errUnableToGetAffectedRowsCount, err)
	}
	if rowsAffected == 0 {
		return nil, xerrors.Errorf(errConnectionWithIDNotFound, connectionID)
	}

	metadata := &connman.ConnectionOperationMetadata{Type: &connman.ConnectionOperationMetadata_Delete{Delete: &connman.DeleteConnectionMetadata{}}}
	op, err := s.insertConnectionOperation(ctx, tx, connectionID, metadata)
	if err != nil {
		return nil, xerrors.Errorf("unable to insert delete operation: %w", err)
	}

	return op, nil
}

func (s *pgStorage) Close() error {
	return s.pg.Close()
}

func (s *pgStorage) insertConnection(ctx context.Context, tx *sqlx.Tx, connection *types.Connection) error {
	logger := s.loggerFactory(ctx)

	query := `
INSERT INTO connection (id, folder_id, name, description, labels, created_by, params)
VALUES (:id, :folder_id, :name, :description, :labels, :created_by, :params)
ON CONFLICT (folder_id, name) DO NOTHING
RETURNING created_at
`
	rows, err := sqlx.NamedQueryContext(ctx, tx, query, connection)
	if err != nil {
		return xerrors.Errorf(errUnableToExecuteQuery, err)
	}
	defer cleanup.Close(rows, logger)

	if !rows.Next() {

		return xerrors.Errorf(errConnectionWithNameAlreadyExists, connection.Name, connection.FolderID)
	}

	if rows.Err() != nil {
		return xerrors.Errorf(errUnableToReadRow, err)
	}

	err = rows.Scan(&connection.CreatedAt)
	if err != nil {
		return xerrors.Errorf(errUnableToScanRow, err)
	}

	return nil
}

func (s *pgStorage) updateConnection(ctx context.Context, connection *types.Connection, tx *sqlx.Tx) error {
	logger := s.loggerFactory(ctx)

	query := `
UPDATE connection
SET updated_at = NOW(),
    name = :name,
    description = :description,
    labels = :labels,
    params = :params
WHERE id = :id
RETURNING updated_at
`
	rows, err := sqlx.NamedQueryContext(ctx, tx, query, connection)
	if err != nil {
		pgErr, ok := err.(*pgconn.PgError)
		if ok && pgErr.Code == "23505" {
			return xerrors.Errorf(errConnectionWithNameAlreadyExists, connection.Name, connection.FolderID)
		}
		return xerrors.Errorf(errUnableToExecuteQuery, err)
	}
	defer cleanup.Close(rows, logger)

	if !rows.Next() {
		return xerrors.Errorf(errConnectionWithIDNotFound, connection.ID)
	}

	if rows.Err() != nil {
		return xerrors.Errorf(errUnableToReadRow, rows.Err())
	}

	err = rows.Scan(&connection.UpdatedAt)
	if err != nil {
		return xerrors.Errorf(errUnableToScanRow, err)
	}

	return nil
}

func (s *pgStorage) insertConnectionOperation(
	ctx context.Context,
	tx *sqlx.Tx,
	connectionID string,
	metadata *connman.ConnectionOperationMetadata,
) (*types.ConnectionOperation, error) {
	logger := s.loggerFactory(ctx)

	userID, err := auth.UserIDFromContext(ctx)
	if err != nil {
		return nil, xerrors.Errorf(auth.ErrUnableToGetUserID, err)
	}

	query := `
INSERT INTO connection_operation (id, connection_id, created_by, done, metadata)
VALUES (:id, :connection_id, :created_by, :done, :metadata)
RETURNING created_at
`
	op := &types.ConnectionOperation{
		ID:           types.GenerateOperationID(),
		ConnectionID: connectionID,
		CreatedAt:    time.Time{},
		UpdatedAt:    nil,
		CreatedBy:    userID,
		Done:         true,
		Metadata:     &types.ConnectionOperationMetadata{ConnectionOperationMetadata: metadata},
	}

	err = s.serializer.Serialize(ctx, op.Metadata)
	if err != nil {
		return nil, xerrors.Errorf("unable to serialize connection operation: %w", err)
	}

	rows, err := sqlx.NamedQueryContext(ctx, tx, query, op)
	if err != nil {
		return nil, xerrors.Errorf(errUnableToExecuteQuery, err)
	}
	defer cleanup.Close(rows, logger)

	if !rows.Next() {
		return nil, xerrors.New("rows set is empty")
	}

	if rows.Err() != nil {
		return nil, xerrors.Errorf(errUnableToReadRow, rows.Err())
	}

	err = rows.Scan(&op.CreatedAt)
	if err != nil {
		return nil, xerrors.Errorf(errUnableToScanRow, err)
	}

	return op, nil
}
