package pg

import (
	"context"
	"database/sql"
	"encoding/json"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
)

var (
	querySelectOperationByID = sqlutil.Stmt{
		Name: "SelectOperationByID",
		Query: `
SELECT
    fmt.operation_id,
    fmt.target_id,
    fmt.cid,
    fmt.cluster_type,
    fmt.env,
    fmt.operation_type,
    fmt.created_by,
    fmt.created_at,
    fmt.started_at,
    fmt.modified_at,
    fmt.status
FROM dbaas.worker_queue q
  JOIN dbaas.clusters c
 USING (cid),
       code.as_operation(q, c) fmt
 WHERE task_id = :operation_id`,
	}
	queryCreateTask = sqlutil.Stmt{
		Name: "CreateTask",
		Query: `
SELECT
    operation_id,
    target_id,
    cid,
    cluster_type,
    env,
    operation_type,
    created_by,
    created_at,
    started_at,
    modified_at,
    status
FROM code.add_operation(
    i_operation_id          => CAST(:task_id AS text),
    i_cid                   => CAST(:cid AS text),
    i_folder_id             => :folder_id,
    i_operation_type        => CAST(:operation_type AS text),
    i_task_type             => CAST(:task_type AS text),
    i_task_args             => CAST(:task_args AS jsonb),
    i_metadata              => CAST(:metadata AS jsonb),
    i_user_id               => CAST(:user_id AS text),
    i_version               => :version,
    i_hidden                => :hidden,
    i_time_limit            => :time_limit,
    i_idempotence_data      => CAST((:idempotence_id, :idempotence_hash) AS code.idempotence_data),
    i_delay_by              => :delay_by,
    i_required_operation_id => :required_operation_id,
    i_rev                   => :rev,
    i_tracing               => :tracing
)`,
	}
)

const taskVersion = 2

func (mdb *metaDB) CreateTask(ctx context.Context, createArgs models.CreateTaskArgs) (models.Operation, error) {
	var op operation
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&op)
	}

	if err := createArgs.Validate(); err != nil {
		return models.Operation{}, err
	}

	taskArgs, err := json.Marshal(createArgs.TaskArgs)
	if err != nil {
		return models.Operation{}, xerrors.Errorf("marshal task args: %w", err)
	}

	metadata, err := json.Marshal(createArgs.Metadata)
	if err != nil {
		return models.Operation{}, xerrors.Errorf("marshal metadata: %w", err)
	}

	timeLimit := &pgtype.Interval{Status: pgtype.Null}
	if createArgs.Timeout.Valid {
		if err := timeLimit.Set(createArgs.Timeout.Duration); err != nil {
			return models.Operation{}, xerrors.Errorf("set timeout: %w", err)
		}
	}

	delayBy := &pgtype.Interval{Status: pgtype.Null}
	if createArgs.DelayBy.Valid {
		if err := delayBy.Set(createArgs.DelayBy.Duration); err != nil {
			return models.Operation{}, xerrors.Errorf("set delay by: %w", err)
		}
	}

	// Either both values are null or both values are set
	var idempotenceID sql.NullString
	idempotenceHash := &pgtype.Bytea{Status: pgtype.Null}
	if !createArgs.SkipIdempotence && createArgs.Idempotence != nil {
		idempotenceID.String = createArgs.Idempotence.ID
		idempotenceID.Valid = true
		if err := idempotenceHash.Set(createArgs.Idempotence.Hash); err != nil {
			return models.Operation{}, xerrors.Errorf("set idempotence hash: %w", err)
		}
	}

	args := map[string]interface{}{
		"task_id":               createArgs.TaskID,
		"cid":                   createArgs.ClusterID,
		"folder_id":             createArgs.FolderID,
		"operation_type":        createArgs.OperationType,
		"task_type":             createArgs.TaskType,
		"task_args":             string(taskArgs),
		"metadata":              string(metadata),
		"user_id":               createArgs.Auth.MustID(),
		"version":               taskVersion,
		"hidden":                createArgs.Hidden,
		"time_limit":            timeLimit,
		"delay_by":              delayBy,
		"required_operation_id": sql.NullString(createArgs.RequiredOperationID),
		"rev":                   createArgs.Revision,
		"idempotence_id":        idempotenceID,
		"idempotence_hash":      idempotenceHash,
		"tracing":               sql.NullString{},
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryCreateTask,
		args,
		parser,
		mdb.l,
	)
	if err != nil {
		return models.Operation{}, err
	}
	if count == 0 {
		return models.Operation{}, sqlerrors.ErrNotFound
	}

	return operationFromDB(op)
}

func (mdb *metaDB) Task(ctx context.Context, taskID string) (models.Operation, error) {
	var op operation
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&op)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectOperationByID,
		map[string]interface{}{
			"operation_id": taskID,
		},
		parser,
		mdb.l,
	)
	if err != nil {
		return models.Operation{}, err
	}
	if count == 0 {
		return models.Operation{}, sqlerrors.ErrNotFound
	}

	return operationFromDB(op)
}
