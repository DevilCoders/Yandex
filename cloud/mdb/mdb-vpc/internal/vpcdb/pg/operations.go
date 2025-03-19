package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	db_models "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/pg/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryInsertNewOperation = sqlutil.Stmt{
		Name: "InsertNewOperation",
		// language=PostgreSQL
		Query: `
INSERT INTO vpc.operations (project_id, description, created_by, metadata, action, cloud_provider, region)
VALUES (:project_id, :description, :created_by, :metadata, :action, :cloud_provider, :region)
RETURNING operation_id`}

	queryOperationToProcess = sqlutil.NewStmt(
		"OperationToProcess",
		// language=PostgreSQL
		`
SELECT *
FROM vpc.operations
WHERE status != 'DONE'
ORDER BY created_by
LIMIT 1
FOR UPDATE
SKIP LOCKED
`,
		db_models.Operation{},
	)

	queryUpdateOperationFields = sqlutil.Stmt{
		Name: "UpdateOperationFields",
		// language=PostgreSQL
		Query: `
UPDATE vpc.operations
SET
    start_time = :start_time,
    finish_time = :finish_time,
    state = :state,
    status = :status
WHERE operation_id = :id`,
	}

	queryOperationByID = sqlutil.Stmt{
		Name: "OperationByID",
		// language=PostgreSQL
		Query: `
SELECT *
FROM vpc.operations
WHERE operation_id = :id
`,
	}
)

func (d *DB) OperationToProcess(ctx context.Context) (models.Operation, error) {
	var dbOp db_models.Operation
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&dbOp)
	}

	c, err := sqlutil.QueryTx(
		ctx,
		queryOperationToProcess,
		map[string]interface{}{},
		parser,
		d.logger,
	)
	if err != nil {
		return models.Operation{}, err
	}
	if c == 0 {
		return models.Operation{}, semerr.NotFound("no operations to process")
	}
	return dbOp.ToInternal()
}

func (d *DB) InsertOperation(
	ctx context.Context,
	projectID string,
	description string,
	createdBy string,
	params models.OperationParams,
	action models.OperationAction,
	provider models.Provider,
	region string,
) (string, error) {
	var operationID string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&operationID)
	}

	dbParams, err := db_models.OperationParamsToDB(params)
	if err != nil {
		return "", err
	}

	_, err = sqlutil.QueryTx(
		ctx,
		queryInsertNewOperation,
		map[string]interface{}{
			"project_id":     projectID,
			"description":    description,
			"created_by":     createdBy,
			"metadata":       string(dbParams),
			"action":         action,
			"cloud_provider": provider,
			"region":         region,
		},
		parser,
		d.logger,
	)

	return operationID, err
}

func (d *DB) UpdateOperationFields(ctx context.Context, op models.Operation) error {
	dbState, err := db_models.OperationStateToDB(op.State)
	if err != nil {
		return xerrors.Errorf("operation state to db: %w", err)
	}

	_, err = sqlutil.QueryTx(
		ctx,
		queryUpdateOperationFields,
		map[string]interface{}{
			"id":          op.ID,
			"status":      op.Status,
			"state":       string(dbState),
			"start_time":  op.StartTime,
			"finish_time": op.FinishTime,
		},
		sqlutil.NopParser,
		d.logger,
	)
	return err
}

func (d *DB) OperationByID(ctx context.Context, operationID string) (models.Operation, error) {
	var dbOp db_models.Operation
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&dbOp)
	}

	c, err := sqlutil.QueryContext(
		ctx,
		d.cluster.MasterPreferredChooser(),
		queryOperationByID,
		map[string]interface{}{
			"id": operationID,
		},
		parser,
		d.logger,
	)
	if err != nil {
		return models.Operation{}, err
	}
	if c == 0 {
		return models.Operation{}, semerr.NotFoundf("no operation with id %q", operationID)
	}
	return dbOp.ToInternal()
}
