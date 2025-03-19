package pg

import (
	"context"
	"database/sql"

	"github.com/gofrs/uuid"
	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	db_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

var (
	queryListInstanceOperations = sqlutil.NewStmt(
		"ListInstanceOperations",
		// language=PostgreSQL
		`
SELECT * FROM cms.instance_operations
WHERE
      status = ANY (:statuses)
`,
		db_models.InstanceOperation{},
	)

	queryInsertNewInstanceOperation = sqlutil.Stmt{
		Name: "InsertNewInstanceOperation",
		// language=PostgreSQL
		Query: `
INSERT INTO cms.instance_operations (operation_type, status, comment, author, instance_id, idempotency_key, operation_state)
VALUES (:operation_type, :status, :comment, :author, :instance_id, :idempotency_key, :operation_state)
ON CONFLICT (idempotency_key, instance_id) DO UPDATE
SET modified_at = now()
RETURNING operation_id`}

	queryGetInstanceOperationByID = sqlutil.NewStmt(
		"GetInstanceOperationById",
		// language=PostgreSQL
		`
SELECT *
FROM cms.instance_operations WHERE operation_id = ANY(:ids_array)`,
		db_models.InstanceOperation{},
	)

	queryInstanceOperationsByStatus = sqlutil.NewStmt(
		"InstanceOperationsByStatus",
		// language=PostgreSQL
		`
SELECT * FROM cms.instance_operations
WHERE status = ANY (:statuses)
ORDER BY created_at
FETCH FIRST :limit ROWS ONLY`,
		db_models.InstanceOperation{},
	)

	queryUpdateInstanceOperationsFields = sqlutil.Stmt{
		Name: "UpdateInstanceOperationsFields",
		// language=PostgreSQL
		Query: `
UPDATE cms.instance_operations
SET
    executed_step_names = :step_names,
    explanation = :explanation,
    operation_log = :log,
    operation_state = :state,
    status = :status,
    modified_at = now()
WHERE operation_id = :id`,
	}

	queryStaleInstanceOperations = sqlutil.NewStmt(
		"StaleInstanceOperations",
		// language=PostgreSQL
		`
SELECT * FROM cms.instance_operations
WHERE
      status = ANY (:statuses)
  AND (
      modified_at - created_at > INTERVAL :in_progress_duration
          OR
      now() - modified_at > INTERVAL :not_work_duration
      )`,
		db_models.InstanceOperation{},
	)
)

func (b *Backend) CreateInstanceOperation(
	ctx context.Context,
	idempotencyKey string,
	operationType models.InstanceOperationType,
	instanceID string,
	comment string,
	author string,
) (string, error) {
	var operationID string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&operationID)
	}

	state, err := db_models.OperationStateToDB(models.DefaultOperationState())
	if err != nil {
		return "", err
	}

	_, err = sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryInsertNewInstanceOperation,
		map[string]interface{}{
			"operation_type":  operationType,
			"status":          models.InstanceOperationStatusNew,
			"comment":         comment,
			"author":          author,
			"instance_id":     instanceID,
			"idempotency_key": idempotencyKey,
			"operation_state": string(state),
		},
		parser,
		b.log,
	)

	return operationID, wrapError(err)
}

func (b *Backend) GetInstanceOperation(ctx context.Context, operationID string) (models.ManagementInstanceOperation, error) {
	if err := validateOperationID(operationID); err != nil {
		return models.ManagementInstanceOperation{}, err
	}

	var op models.ManagementInstanceOperation
	parser := func(rows *sqlx.Rows) error {
		var dbOp db_models.InstanceOperation
		err := rows.StructScan(&dbOp)
		if err != nil {
			return err
		}
		op, err = dbOp.ToInternal(b.log)
		return err
	}

	var pgIds pgtype.TextArray
	err := pgIds.Set([]string{operationID})
	if err != nil {
		return models.ManagementInstanceOperation{}, err
	}

	count, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryGetInstanceOperationByID,
		map[string]interface{}{
			"ids_array": pgIds,
		},
		parser,
		b.log,
	)
	if err != nil {
		return op, err
	}

	if count == 0 {
		return op, semerr.NotFoundf("operation %q doesn't exist", operationID)
	}

	return op, err
}

func (b *Backend) InstanceOperationsToProcess(ctx context.Context, limit int) ([]models.ManagementInstanceOperation, error) {
	return b.instanceOperationsByStatus(ctx, sql.NullInt64{Int64: int64(limit), Valid: true}, models.InstanceOperationStatusOkPending, models.InstanceOperationStatusRejectPending, models.InstanceOperationStatusNew, models.InstanceOperationStatusInProgress)
}

func (b *Backend) InstanceOperationsToAlarm(ctx context.Context) ([]models.ManagementInstanceOperation, error) {
	return b.instanceOperationsByStatus(ctx, sql.NullInt64{}, models.InstanceOperationStatusRejected)
}

func (b *Backend) instanceOperationsByStatus(
	ctx context.Context,
	limit sql.NullInt64,
	statuses ...models.InstanceOperationStatus,
) ([]models.ManagementInstanceOperation, error) {
	var results []models.ManagementInstanceOperation
	parser := func(rows *sqlx.Rows) error {
		var dbOp db_models.InstanceOperation
		if err := rows.StructScan(&dbOp); err != nil {
			return err
		}
		r, err := dbOp.ToInternal(b.log)
		if err != nil {
			return err
		}
		results = append(results, r)
		return nil
	}

	var pgStatuses pgtype.TextArray
	if err := pgStatuses.Set(statuses); err != nil {
		return nil, err
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryInstanceOperationsByStatus,
		map[string]interface{}{
			"limit":    limit,
			"statuses": pgStatuses,
		},
		parser,
		b.log,
	)

	return results, err
}

func (b *Backend) UpdateInstanceOperationFields(ctx context.Context, operation models.ManagementInstanceOperation) error {
	dboml, err := db_models.OperationStateToDB(operation.State)
	if err != nil {
		return err
	}
	snames := make([]string, len(operation.ExecutedStepNames))
	copy(snames, operation.ExecutedStepNames)
	var pgSteps pgtype.TextArray
	if err = pgSteps.Set(snames); err != nil {
		return err
	}
	_, err = sqlutil.QueryTx(
		ctx,
		queryUpdateInstanceOperationsFields,
		map[string]interface{}{
			"step_names":  pgSteps,
			"id":          operation.ID,
			"status":      operation.Status,
			"explanation": operation.Explanation,
			"log":         operation.Log,
			"state":       string(dboml),
		},
		sqlutil.NopParser,
		b.log,
	)
	return err
}

func (b *Backend) StaleInstanceOperations(ctx context.Context) ([]models.ManagementInstanceOperation, error) {
	var results []models.ManagementInstanceOperation
	parser := func(rows *sqlx.Rows) error {
		var dbOp db_models.InstanceOperation
		if err := rows.StructScan(&dbOp); err != nil {
			return err
		}
		r, err := dbOp.ToInternal(b.log)
		if err != nil {
			return err
		}
		results = append(results, r)
		return nil
	}

	statuses := []models.InstanceOperationStatus{
		models.InstanceOperationStatusInProgress,
		models.InstanceOperationStatusNew,
	}
	var pgStatuses pgtype.TextArray
	if err := pgStatuses.Set(statuses); err != nil {
		return nil, err
	}

	_, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryStaleInstanceOperations,
		map[string]interface{}{
			"statuses":             pgStatuses,
			"in_progress_duration": "1 day",
			"not_work_duration":    "10 minute",
		},
		parser,
		b.log,
	)

	return results, err
}

func (b *Backend) ListInstanceOperations(ctx context.Context) ([]models.ManagementInstanceOperation, error) {
	var results []models.ManagementInstanceOperation
	parser := func(rows *sqlx.Rows) error {
		var dbOp db_models.InstanceOperation
		if err := rows.StructScan(&dbOp); err != nil {
			return err
		}
		r, err := dbOp.ToInternal(b.log)
		if err != nil {
			return err
		}
		results = append(results, r)
		return nil
	}

	statuses := []models.InstanceOperationStatus{
		models.InstanceOperationStatusInProgress,
	}
	var pgStatuses pgtype.TextArray
	if err := pgStatuses.Set(statuses); err != nil {
		return nil, err
	}

	_, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryListInstanceOperations,
		map[string]interface{}{
			"statuses": pgStatuses,
		},
		parser,
		b.log,
	)

	return results, err
}

func validateOperationID(opID string) error {
	_, err := uuid.FromString(opID)
	if err != nil {
		return semerr.InvalidInputf("operationID must be valid UUID, got %q", opID)
	}

	return nil
}
