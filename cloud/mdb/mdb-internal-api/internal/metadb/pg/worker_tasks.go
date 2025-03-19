package pg

import (
	"context"
	"database/sql"
	"encoding/json"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	querySelectOperationByID = sqlutil.Stmt{
		Name: "SelectOperationByID",
		Query: `
SELECT fmt.*
  FROM dbaas.worker_queue q
  JOIN dbaas.clusters c
 USING (cid),
       code.as_operation(q, c) fmt
 WHERE task_id = :operation_id`,
	}

	querySelectOperationIDByIdempotenceID = sqlutil.Stmt{
		Name: "SelectOperationIDByIdempotenceID",
		Query: `
SELECT operation_id AS id,
       request_hash
  FROM code.get_operation_id_by_idempotence(
	i_idempotence_id => :idempotence_id,
	i_folder_id      => :folder_id,
	i_user_id        => :user_id
  )`,
	}

	querySelectOperationsByFolderID = sqlutil.Stmt{
		Name: "SelectOperationsByFolderID",
		// language=PostgreSQL
		Query: `
SELECT  operation_id,
		target_id,
		cluster_type,
		env,
		operation_type,
		created_by,
		created_at,
		started_at,
		modified_at,
		status,
		metadata,
		hidden,
		errors
 FROM code.get_operations(
	 i_folder_id             => :folder_id,
	 i_cid                   => CAST(:cid AS text),
	 i_limit                 => :limit,
	 i_env                   => :env,
	 i_cluster_type          => :cluster_type,
	 i_type                  => :type,
	 i_created_by            => :created_by,
	 i_page_token_id         => :page_token_id,
	 i_page_token_create_ts  => :page_token_create_ts,
	 i_include_hidden        => :include_hidden
 )
 `,
	}

	querySelectOperationsByClusterID = sqlutil.Stmt{
		Name: "SelectOperationsByClusterID",
		// language=PostgreSQL
		Query: `
SELECT  operation_id,
		target_id,
		cluster_type,
		env,
		operation_type,
		created_by,
		created_at,
		started_at,
		modified_at,
		status,
		metadata,
		hidden,
		errors
 FROM code.get_operations(
	 i_folder_id             => :folder_id,
	 i_cid                   => CAST(:cid AS text),
	 i_limit                 => :limit
 )
 `,
	}

	querySelectTheMostRecentInitiatedByUserOperationByClusterID = sqlutil.Stmt{
		Name: "SelectTheMostRecentOperationInitiatedByUsersByClusterID",
		// language=PostgreSQL
		Query: `
SELECT fmt.*
  FROM dbaas.worker_queue q
  JOIN dbaas.clusters c USING (cid),
	   code.as_operation(q, c) fmt
 WHERE q.cid = CAST(:cid AS text)
   AND NOT q.hidden
   AND code.task_type_action(task_type) NOT IN (
	  'cluster-maintenance',
	  'cluster-resetup',
	  'cluster-offline-resetup')
ORDER BY create_rev DESC,
		 acquire_rev NULLS LAST,
		 required_task_id NULLS FIRST
FETCH FIRST ROW ONLY
 `,
	}

	querySelectRunningTaskID = sqlutil.Stmt{
		Name: "SelectRunningTaskID",
		Query: `
SELECT task_id AS operation_id
FROM dbaas.worker_queue
WHERE result IS NULL
	AND cid = :cid`,
	}

	querySelectFailedTaskID = sqlutil.Stmt{
		Name: "SelectFailedTaskID",
		Query: `
SELECT task_id AS operation_id
FROM dbaas.worker_queue q
JOIN dbaas.clusters c ON (q.cid = c.cid)
WHERE CAST(c.status AS text) LIKE '%%ERROR'
	AND q.result = false
	AND q.unmanaged = false
	AND c.cid = :cid
ORDER BY q.end_ts ASC
LIMIT 1`,
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
    status,
    metadata,
    args,
    hidden,
    required_operation_id,
    errors
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

	queryCreateFinishedTask = sqlutil.Stmt{
		Name: "CreateFinishedTask",
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
    status,
    metadata,
    args,
    hidden,
    required_operation_id,
    errors
FROM code.add_finished_operation(
	i_operation_id     => CAST(:task_id AS text),
	i_cid              => CAST(:cid AS text),
	i_folder_id        => :folder_id,
	i_operation_type   => CAST(:operation_type AS text),
	i_metadata         => CAST(:metadata AS jsonb),
	i_user_id          => CAST(:user_id AS text),
	i_idempotence_data => CAST((:idempotence_id, :idempotence_hash) AS code.idempotence_data),
	i_version          => :version,
	i_rev              => :rev
)`,
	}

	queryCreateFinishedTaskAtCurrentRev = sqlutil.Stmt{
		Name: "CreateFinishedTaskAtCurrentRev",
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
    status,
    metadata,
    args,
    hidden,
    required_operation_id,
    errors
FROM code.add_finished_operation_for_current_rev(
	i_operation_id     => CAST(:task_id AS text),
	i_cid              => CAST(:cid AS text),
	i_folder_id        => :folder_id,
	i_operation_type   => CAST(:operation_type AS text),
	i_metadata         => CAST(:metadata AS jsonb),
	i_user_id          => CAST(:user_id AS text),
	i_idempotence_data => CAST((:idempotence_id, :idempotence_hash) AS code.idempotence_data),
	i_version          => :version,
	i_rev              => :rev
)`,
	}

	queryCreateWorkerQueueEvent = sqlutil.Stmt{
		Name: "CreateWorkerQueueEvent",
		Query: `
INSERT INTO dbaas.worker_queue_events (task_id, data)
VALUES (:task_id, :data)`,
	}

	queryCreateSearchQueueDoc = sqlutil.Stmt{
		Name: "CreateSearchQueueDoc",
		Query: `
INSERT INTO dbaas.search_queue (doc)
VALUES (:doc)`,
	}
)

func (b *Backend) OperationByID(ctx context.Context, oid string) (operations.Operation, error) {
	var op operation
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&op)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectOperationByID,
		map[string]interface{}{
			"operation_id": oid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return operations.Operation{}, err
	}
	if count == 0 {
		return operations.Operation{}, sqlerrors.ErrNotFound
	}

	return operationFromDB(op)
}

func (b *Backend) OperationIDByIdempotenceID(ctx context.Context, idempID, userID string, folderID int64) (string, []byte, error) {
	var res struct {
		ID   string `db:"id"`
		Hash []byte `db:"request_hash"`
	}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&res)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectOperationIDByIdempotenceID,
		map[string]interface{}{
			"idempotence_id": idempID,
			"user_id":        userID,
			"folder_id":      folderID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return "", nil, err
	}
	if count == 0 {
		return "", nil, sqlerrors.ErrNotFound
	}

	return res.ID, res.Hash, nil
}

func (b *Backend) OperationsByFolderID(ctx context.Context, folderID int64, args models.ListOperationsArgs) ([]operations.Operation, error) {
	var ops []operations.Operation
	parser := func(rows *sqlx.Rows) error {
		var opDB operation
		if err := rows.StructScan(&opDB); err != nil {
			return err
		}
		op, err := operationFromDB(opDB)
		if err != nil {
			return err
		}
		ops = append(ops, op)
		return nil
	}

	clusterType := pgtype.Text{Status: pgtype.Null}
	if args.ClusterType != clusters.TypeUnknown {
		clusterType = pgtype.Text{
			String: args.ClusterType.Stringified(),
			Status: pgtype.Present,
		}
	}

	envValid := args.Environment != "" && args.Environment != environment.SaltEnvInvalid
	opTypeValid := args.Type != ""

	_, err := sqlutil.QueryTx(
		ctx,
		querySelectOperationsByFolderID,
		map[string]interface{}{
			"folder_id":            folderID,
			"cid":                  sql.NullString(args.ClusterID),
			"limit":                sql.NullInt64{Int64: args.Limit.Int64, Valid: args.Limit.Valid},
			"env":                  sql.NullString{String: string(args.Environment), Valid: envValid},
			"cluster_type":         clusterType,
			"type":                 sql.NullString{String: string(args.Type), Valid: opTypeValid},
			"created_by":           sql.NullString(args.CreatedBy),
			"page_token_id":        sql.NullString(args.PageTokenID),
			"page_token_create_ts": sql.NullTime(args.PageTokenCreateTS),
			"include_hidden":       sql.NullBool(args.IncludeHidden),
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return ops, nil
}

func (b *Backend) OperationsByClusterID(ctx context.Context, cid string, folderID int64, offset int64, pageSize int64) ([]operations.Operation, error) {
	var ops []operations.Operation
	parser := func(rows *sqlx.Rows) error {
		var opDB operation
		if err := rows.StructScan(&opDB); err != nil {
			return err
		}
		op, err := operationFromDB(opDB)
		if err != nil {
			return err
		}
		ops = append(ops, op)
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		querySelectOperationsByClusterID,
		map[string]interface{}{
			"folder_id": folderID,
			"cid":       cid,
			"limit":     pageSize,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return ops, nil
}

func (b *Backend) MostRecentInitiatedByUserOperationByClusterID(ctx context.Context, cid string) (operations.Operation, error) {
	var op operations.Operation
	parser := func(rows *sqlx.Rows) error {
		var opDB operation
		if err := rows.StructScan(&opDB); err != nil {
			return err
		}
		opParsed, err := operationFromDB(opDB)
		if err != nil {
			return err
		}
		op = opParsed
		return nil
	}

	rowCount, err := sqlutil.QueryTx(
		ctx,
		querySelectTheMostRecentInitiatedByUserOperationByClusterID,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return operations.Operation{}, err
	}
	if rowCount == 0 {
		return operations.Operation{}, sqlerrors.ErrNotFound
	}

	return op, nil
}

func (b *Backend) RunningTaskID(ctx context.Context, cid string) (string, error) {
	return b.selectTaskID(ctx, cid, querySelectRunningTaskID)
}

func (b *Backend) FailedTaskID(ctx context.Context, cid string) (string, error) {
	return b.selectTaskID(ctx, cid, querySelectFailedTaskID)
}

func (b *Backend) selectTaskID(ctx context.Context, cid string, query sqlutil.Stmt) (string, error) {
	var id string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&id)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		query,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return "", err
	}
	if count == 0 {
		return "", sqlerrors.ErrNotFound
	}

	return id, nil
}

const taskVersion = 2

func (b *Backend) CreateTask(ctx context.Context, createArgs tasks.CreateTaskArgs, tracingCarrier opentracing.TextMapCarrier) (operations.Operation, error) {
	var op operation
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&op)
	}

	timeLimit := &pgtype.Interval{Status: pgtype.Null}
	if createArgs.Timeout.Valid {
		if err := timeLimit.Set(createArgs.Timeout.Duration); err != nil {
			return operations.Operation{}, xerrors.Errorf("set timeout: %w", err)
		}
	}

	taskArgs, err := json.Marshal(createArgs.TaskArgs)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("marshal task args: %w", err)
	}

	// TODO: debug &pgtype.JSON{} failing
	/*metadata := &pgtype.JSON{}
	if err := metadata.Set(createArgs.Metadata); err != nil {
		return operations.Operation{}, xerrors.Errorf("error while converting metadata for task creation: %w", err)
	}*/
	metadata, err := json.Marshal(createArgs.Metadata)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("marshal metadata: %w", err)
	}

	delayBy := &pgtype.Interval{Status: pgtype.Null}
	if createArgs.DelayBy.Valid {
		if err := delayBy.Set(createArgs.DelayBy.Duration); err != nil {
			return operations.Operation{}, xerrors.Errorf("set delay by: %w", err)
		}
	}

	// Either both values are null or both values are set
	var idempotenceID sql.NullString
	idempotenceHash := &pgtype.Bytea{Status: pgtype.Null}
	if !createArgs.SkipIdempotence && createArgs.Idempotence != nil {
		idempotenceID.String = createArgs.Idempotence.ID
		idempotenceID.Valid = true
		if err := idempotenceHash.Set(createArgs.Idempotence.Hash); err != nil {
			return operations.Operation{}, xerrors.Errorf("set idempotence hash: %w", err)
		}
	}

	pgTracingCarrier, err := tracing.MarshalTextMapCarrier(tracingCarrier)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("tracing carrier: %w", err)
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
		"tracing":               string(pgTracingCarrier),
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryCreateTask,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return operations.Operation{}, err
	}
	if count == 0 {
		return operations.Operation{}, sqlerrors.ErrNotFound
	}

	return operationFromDB(op)
}

func (b *Backend) createFinishedTask(ctx context.Context, createArgs tasks.CreateFinishedTaskArgs, query sqlutil.Stmt) (operations.Operation, error) {
	var op operation
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&op)
	}

	metadata, err := json.Marshal(createArgs.Metadata)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("error while converting metadata for task creation: %w", err)
	}

	// Either both values are null or both values are set
	var idempotenceID sql.NullString
	idempotenceHash := &pgtype.Bytea{Status: pgtype.Null}
	if createArgs.Idempotence != nil {
		idempotenceID.String = createArgs.Idempotence.ID
		idempotenceID.Valid = true
		if err := idempotenceHash.Set(createArgs.Idempotence.Hash); err != nil {
			return operations.Operation{}, xerrors.Errorf("set idempotence hash: %w", err)
		}
	}

	args := map[string]interface{}{
		"task_id":          createArgs.TaskID,
		"cid":              createArgs.ClusterID,
		"folder_id":        createArgs.FolderID,
		"operation_type":   createArgs.OperationType,
		"metadata":         string(metadata),
		"user_id":          createArgs.Auth.MustID(),
		"version":          taskVersion,
		"rev":              createArgs.Revision,
		"idempotence_id":   idempotenceID,
		"idempotence_hash": idempotenceHash,
	}

	count, err := sqlutil.QueryTx(
		ctx,
		query,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return operations.Operation{}, err
	}
	if count == 0 {
		return operations.Operation{}, sqlerrors.ErrNotFound
	}

	return operationFromDB(op)
}

func (b *Backend) CreateFinishedTask(ctx context.Context, createArgs tasks.CreateFinishedTaskArgs) (operations.Operation, error) {
	return b.createFinishedTask(ctx, createArgs, queryCreateFinishedTask)
}

func (b *Backend) CreateFinishedTaskAtCurrentRev(ctx context.Context, createArgs tasks.CreateFinishedTaskArgs) (operations.Operation, error) {
	return b.createFinishedTask(ctx, createArgs, queryCreateFinishedTaskAtCurrentRev)
}

func (b *Backend) CreateWorkerQueueEvent(ctx context.Context, taskID string, data []byte) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryCreateWorkerQueueEvent,
		map[string]interface{}{
			"task_id": taskID,
			"data":    string(data),
		},
		sqlutil.NopParser,
		b.logger,
	)
	return err
}

func (b *Backend) CreateSearchQueueDoc(ctx context.Context, doc []byte) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryCreateSearchQueueDoc,
		map[string]interface{}{
			"doc": string(doc),
		},
		sqlutil.NopParser,
		b.logger,
	)
	return err
}
