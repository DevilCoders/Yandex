package pg

import (
	"context"
	"encoding/json"
	"fmt"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	db_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/settings"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryActiveRequests = sqlutil.NewStmt(
		"GetActiveRequests",
		// language=PostgreSQL
		`SELECT * FROM cms.requests WHERE is_deleted = false`,
		db_models.Request{},
	)
	queryMarkRequestsDeleted = sqlutil.Stmt{
		Name: "MarkRequestsDeletedByTaskID",
		// language=PostgreSQL
		Query: "UPDATE cms.requests SET is_deleted = true WHERE request_ext_id = ANY(:request_ext_ids_array)",
	}
	queryRequestsByTaskID = sqlutil.NewStmt(
		"GetExistingRequestsTaskIDsByTaskId",
		// language=PostgreSQL
		`SELECT * FROM cms.requests WHERE request_ext_id = ANY(:request_ext_ids_array) AND is_deleted = false`,
		db_models.Request{},
	)
	queryRequestsByID = sqlutil.NewStmt(
		"GetRequestsById",
		// language=PostgreSQL
		`SELECT * FROM cms.requests WHERE id = ANY(:request_ids_array) AND is_deleted = false`,
		db_models.Request{},
	)
	queryRequestsWithDeletedByID = sqlutil.NewStmt(
		"GetRequestsWithDeletedById",
		// language=PostgreSQL
		`SELECT * FROM cms.requests WHERE id = ANY(:request_ids_array)`,
		db_models.Request{},
	)
	queryInsertNewRequest = sqlutil.Stmt{
		Name: "InsertNewRequest",
		// language=PostgreSQL
		Query: `
INSERT INTO cms.requests (name, request_ext_id, status, comment, author, request_type, fqdns, extra, failure_type, scenario_info)
VALUES (:name, :request_ext_id, :status, :comment, :author, :request_type, :fqdns, :extra, :failure_type, :scenario_info)
ON CONFLICT (request_ext_id) DO UPDATE
SET is_deleted = false, resolved_at = NULL, resolved_by = NULL, status = :status, resolve_explanation = ''
RETURNING id`}
	queryRequestsToConsider = sqlutil.NewStmt(
		"RequestsToConsider",
		// language=PostgreSQL
		`SELECT * FROM cms.requests WHERE resolved_at IS NULL AND is_deleted = false AND created_at < now() - make_interval(mins => :threshold)`,
		db_models.Request{},
	)
	queryRequestsAnalysed = sqlutil.Stmt{
		Name: "MarkRequestsAnalysed",
		// language=PostgreSQL
		Query: `
UPDATE cms.requests
SET
    analysed_by = :by
WHERE id = ANY(:request_ids_array)`,
	}
	queryGiveRequestsToWalle = sqlutil.Stmt{
		Name: "MarkRequestsGivenToWalle",
		// language=PostgreSQL
		Query: `
UPDATE cms.requests
SET
    resolved_at = now(),
    resolved_by = :by,
    status = :status
WHERE id = ANY(:request_ids_array)`,
	}
	queryCameBackFromWalle = sqlutil.Stmt{
		Name: "MarkCameBackFromWalle",
		// language=PostgreSQL
		Query: `
UPDATE cms.requests
SET
    came_back_at = now(),
    is_deleted = true
WHERE id = ANY(:request_ids_array)`,
	}
	queryFinishAfterWalle = sqlutil.Stmt{
		Name: "MarkRequestFinished",
		// language=PostgreSQL
		Query: `
UPDATE cms.requests
SET
    done_at = now()
WHERE id = ANY(:request_ids_array)`,
	}
	queryUpdateRequestFields = sqlutil.Stmt{
		Name: "UpdateRequestFields",
		// language=PostgreSQL
		Query: `
UPDATE cms.requests
SET
    resolve_explanation = :expl
WHERE id = :id`,
	}
)

func (b *Backend) GetRequests(ctx context.Context) ([]models.ManagementRequest, error) {
	var results []models.ManagementRequest
	parser := func(rows *sqlx.Rows) error {
		var dbTask db_models.Request
		if err := rows.StructScan(&dbTask); err != nil {
			return err
		}

		request, err := dbTask.ToInternal()
		if err != nil {
			return err
		}

		results = append(results, request)
		return nil
	}

	args := map[string]interface{}{}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryActiveRequests,
		args,
		parser,
		b.log,
	); err != nil {
		return nil, err
	}

	return results, nil
}

func (b *Backend) GetRequestsToConsider(ctx context.Context, threshold int) ([]models.ManagementRequest, error) {
	var results []models.ManagementRequest
	parser := func(rows *sqlx.Rows) error {
		var dbTask db_models.Request
		if err := rows.StructScan(&dbTask); err != nil {
			return err
		}

		request, err := dbTask.ToInternal()
		if err != nil {
			return err
		}

		results = append(results, request)
		return nil
	}

	args := map[string]interface{}{
		"threshold": threshold,
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryRequestsToConsider,
		args,
		parser,
		b.log,
	); err != nil {
		return nil, err
	}

	return results, nil
}

func (b *Backend) MarkRequestsDeletedByTaskID(ctx context.Context, taskIDs []string) ([]string, error) {
	existingTaskIDsMap, err := b.GetRequestsByTaskID(ctx, taskIDs)
	if err != nil {
		return nil, err
	}
	var deletedTaskIDs []string
	for tID := range existingTaskIDsMap {
		deletedTaskIDs = append(deletedTaskIDs, tID)
	}

	var pgTaskIds pgtype.TextArray
	err = pgTaskIds.Set(taskIDs)
	if err != nil {
		return nil, err
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryMarkRequestsDeleted,
		map[string]interface{}{
			"request_ext_ids_array": pgTaskIds,
		},
		sqlutil.NopParser,
		b.log,
	); err != nil {
		return deletedTaskIDs, err
	}
	return deletedTaskIDs, nil
}

// Idempotent regarding external ids. When task already exists, clears its status and makes it visible to duty.
func (b *Backend) CreateRequests(ctx context.Context, createUs []cmsdb.RequestToCreate) (models.RequestStatus, error) {
	var pgFqdn pgtype.TextArray

	for _, create := range createUs {
		if err := pgFqdn.Set(create.Fqnds); err != nil {
			return models.RequestStatus(""), err
		}
		jBytes, err := json.Marshal(create.Extra)
		if err != nil {
			return models.RequestStatus(""), err
		}

		scenario, err := json.Marshal(create.ScenarioInfo)
		if err != nil {
			return "", xerrors.Errorf("marshal scenario info: %w", err)
		}

		args := map[string]interface{}{
			"request_ext_id": create.ExtID,
			"name":           create.Name,
			"status":         models.StatusInProcess,
			"comment":        create.Comment,
			"author":         create.Author,
			"fqdns":          pgFqdn,
			"request_type":   create.RequestType,
			"extra":          string(jBytes),
			"failure_type":   create.FailureType,
			"scenario_info":  string(scenario),
		}

		if _, err := sqlutil.QueryContext(
			ctx,
			b.cluster.PrimaryChooser(),
			queryInsertNewRequest,
			args,
			sqlutil.NopParser,
			b.log,
		); err != nil {
			return models.RequestStatus(""), err
		}
	}

	return models.StatusInProcess, nil
}

func (b *Backend) GetRequestsByTaskID(ctx context.Context, taskIDs []string) (map[string]models.ManagementRequest, error) {
	results := map[string]models.ManagementRequest{}
	parser := func(rows *sqlx.Rows) error {
		var task db_models.Request

		if err := rows.StructScan(&task); err != nil {
			return err
		}
		request, err := task.ToInternal()

		if err != nil {
			return err
		}
		results[request.ExtID] = request
		return nil
	}

	var pgTaskIds pgtype.TextArray
	err := pgTaskIds.Set(taskIDs)
	if err != nil {
		return nil, err
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryRequestsByTaskID,
		map[string]interface{}{
			"request_ext_ids_array": pgTaskIds,
		},
		parser,
		b.log,
	); err != nil {
		return nil, err
	}
	return results, nil
}

func (b *Backend) GetRequestsByID(ctx context.Context, ids []int64) (map[int64]models.ManagementRequest, error) {
	results := map[int64]models.ManagementRequest{}
	parser := func(rows *sqlx.Rows) error {
		var task db_models.Request

		if err := rows.StructScan(&task); err != nil {
			return err
		}
		request, err := task.ToInternal()

		if err != nil {
			return err
		}
		results[request.ID] = request
		return nil
	}

	var pgIds pgtype.Int8Array
	err := pgIds.Set(ids)
	if err != nil {
		return nil, err
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryRequestsByID,
		map[string]interface{}{
			"request_ids_array": pgIds,
		},
		parser,
		b.log,
	); err != nil {
		return nil, err
	}
	return results, nil
}

func (b *Backend) GetRequestsWithDeletedByID(ctx context.Context, ids []int64) (map[int64]models.ManagementRequest, error) {
	results := map[int64]models.ManagementRequest{}
	parser := func(rows *sqlx.Rows) error {
		var task db_models.Request

		if err := rows.StructScan(&task); err != nil {
			return err
		}
		request, err := task.ToInternal()

		if err != nil {
			return err
		}
		results[request.ID] = request
		return nil
	}

	var pgIds pgtype.Int8Array
	err := pgIds.Set(ids)
	if err != nil {
		return nil, err
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryRequestsWithDeletedByID,
		map[string]interface{}{
			"request_ids_array": pgIds,
		},
		parser,
		b.log,
	); err != nil {
		return nil, err
	}
	return results, nil
}

func (b *Backend) MarkRequestsResolvedByAutoDuty(ctx context.Context, ds []models.AutomaticDecision) error {

	for _, c := range ds {
		moveTo, ok := models.AutoDecisionStatusMap[c.Status]
		if !ok {
			return xerrors.New(fmt.Sprintf("cannot handle status %s", c.Status))
		}

		var pgIDs pgtype.Int8Array
		err := pgIDs.Set([]int64{c.RequestID})
		if err != nil {
			return err
		}

		args := map[string]interface{}{
			"request_ids_array": pgIDs,
			"by":                settings.CMSRobotLogin,
			"status":            moveTo,
		}

		if _, err := sqlutil.QueryContext(
			ctx,
			b.cluster.PrimaryChooser(),
			queryGiveRequestsToWalle,
			args,
			sqlutil.NopParser,
			b.log,
		); err != nil {
			return err
		}
	}
	return nil
}

func (b *Backend) MarkRequestsFinishedByAutoDuty(ctx context.Context, ds []models.AutomaticDecision) error {
	var pgIDs pgtype.Int8Array
	reqIDs := make([]int64, len(ds))
	for i, c := range ds {
		reqIDs[i] = c.RequestID
	}
	if err := pgIDs.Set(reqIDs); err != nil {
		return fmt.Errorf("failed to set reqIDs %w", err)
	}
	args := map[string]interface{}{
		"request_ids_array": pgIDs,
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryFinishAfterWalle,
		args,
		sqlutil.NopParser,
		b.log,
	); err != nil {
		return err
	}
	return nil
}

func (b *Backend) MarkRequestsCameBack(ctx context.Context, ds []models.AutomaticDecision) error {
	var pgIDs pgtype.Int8Array
	reqIDs := make([]int64, len(ds))
	for i, c := range ds {
		reqIDs[i] = c.RequestID
	}
	if err := pgIDs.Set(reqIDs); err != nil {
		return fmt.Errorf("failed to set reqIDs %w", err)
	}
	args := map[string]interface{}{
		"request_ids_array": pgIDs,
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryCameBackFromWalle,
		args,
		sqlutil.NopParser,
		b.log,
	); err != nil {
		return err
	}
	return nil
}

func (b *Backend) MarkRequestsAnalysedByAutoDuty(ctx context.Context, ds []models.AutomaticDecision) error {
	var pgIDs pgtype.Int8Array
	reqIDs := make([]int64, len(ds))
	for i, c := range ds {
		reqIDs[i] = c.RequestID
	}
	if err := pgIDs.Set(reqIDs); err != nil {
		return fmt.Errorf("failed to set reqIDs %w", err)
	}

	args := map[string]interface{}{
		"by":                settings.CMSRobotLogin,
		"request_ids_array": pgIDs,
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryRequestsAnalysed,
		args,
		sqlutil.NopParser,
		b.log,
	); err != nil {
		return err
	}

	return nil
}

func (b *Backend) UpdateRequestFields(ctx context.Context, r models.ManagementRequest) error {
	_, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryUpdateRequestFields,
		map[string]interface{}{
			"id":   r.ID,
			"expl": r.ResolveExplanation,
		},
		sqlutil.NopParser,
		b.log,
	)
	return err
}
