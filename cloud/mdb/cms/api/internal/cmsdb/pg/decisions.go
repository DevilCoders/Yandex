package pg

import (
	"context"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"
	"golang.org/x/xerrors"

	db_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

var (
	queryDecisionsToAnalyse = sqlutil.NewStmt(
		"GetDecisionsToAnalyse",
		// language=PostgreSQL
		`
SELECT d.*
FROM cms.decisions d
INNER JOIN cms.requests r
ON d.request_id = r.id
WHERE
      d.status IN ('new', 'processing', 'wait', 'escalated')
  AND NOT r.is_deleted
  AND d.id != ALL(:skip_ids_array)
ORDER BY r.created_at ASC
LIMIT 1
FOR UPDATE
SKIP LOCKED`,
		db_models.Decision{},
	)
	queryDecisionsToLetGo = sqlutil.NewStmt(
		"GetDecisionsToLetGo",
		// language=PostgreSQL
		`
SELECT d.*
FROM cms.decisions d
INNER JOIN cms.requests r
ON d.request_id = r.id
WHERE
      d.status = 'ok'
  AND NOT r.is_deleted
  AND d.id != ALL(:skip_ids_array)
ORDER BY r.created_at ASC
LIMIT 1
FOR UPDATE
SKIP LOCKED`,
		db_models.Decision{},
	)
	queryDecisionsToReturnFromWalle = sqlutil.NewStmt(
		"GetDecisionsToReturnFromWalle",
		// language=PostgreSQL
		`
SELECT d.*
FROM cms.decisions d
INNER JOIN cms.requests r
ON d.request_id = r.id
WHERE
      d.status = 'at-wall-e'
  AND NOT r.is_deleted
  AND d.id != ALL(:skip_ids_array)
ORDER BY r.created_at ASC
LIMIT 1
FOR UPDATE
SKIP LOCKED`,
		db_models.Decision{},
	)
	queryDecisionsToFinishAfterWalle = sqlutil.NewStmt(
		"GetDecisionsToFinishAfterWalle",
		// language=PostgreSQL
		`
SELECT d.*
FROM cms.decisions d
INNER JOIN cms.requests r
ON d.request_id = r.id
WHERE
      d.status = 'before-done'
      AND d.id != ALL(:skip_ids_array)
ORDER BY r.created_at ASC
LIMIT 1
FOR UPDATE
SKIP LOCKED`,
		db_models.Decision{},
	)
	queryDecisionsToCleanup = sqlutil.NewStmt(
		"GetDecisionsToCleanup",
		// language=PostgreSQL
		`
SELECT d.*
FROM cms.decisions d
INNER JOIN cms.requests r
ON d.request_id = r.id
WHERE
      d.status = 'cleanup'
	  AND d.id != ALL(:skip_ids_array)
ORDER BY r.created_at ASC
LIMIT 1
FOR UPDATE
SKIP LOCKED`,
		db_models.Decision{},
	)
	queryDecisionsByID = sqlutil.NewStmt(
		"GetDecisionsByID",
		// language=PostgreSQL
		`
SELECT *
FROM cms.decisions
WHERE id = ANY(:decision_ids_array)`,
		db_models.Decision{},
	)
	queryMoveDecisionToStatus = sqlutil.Stmt{
		Name: "MoveDecisionToStatus",
		// language=PostgreSQL
		Query: "UPDATE cms.decisions SET status = :status, decided_at = now() WHERE id = ANY(:decision_ids_array)",
	}
	queryUpdateDecisionFields = sqlutil.Stmt{
		Name: "UpdateDecisionFields",
		// language=PostgreSQL
		Query: `
UPDATE cms.decisions
SET
    explanation = :expl,
    mutations_log = :mutat,
    after_walle_log = :aftlog,
    cleanup_log = :cleanuplog,
    ops_metadata_log = :oplog
WHERE id = :id`,
	}
	querySetAutoDutyResolution = sqlutil.Stmt{
		Name: "SetAutoDutyResolution",
		// language=PostgreSQL
		Query: "UPDATE cms.decisions SET ad_resolution = :r WHERE id = ANY(:decision_ids_array)",
	}
	queryInsertDecision = sqlutil.Stmt{
		Name: "InsertDecision",
		// language=PostgreSQL
		Query: `
INSERT INTO cms.decisions
    (request_id,
     status,
     explanation,
     ad_resolution,
     mutations_log,
	 ops_metadata_log,
     after_walle_log,
     cleanup_log)
VALUES (:request_id,
        :status,
        :explanation,
        'unknown',
        '',
        :ops_metadata,
        :finish_log,
        :cleanup_log
        )
RETURNING id`,
	}
	queryDecisionsByRequestID = sqlutil.NewStmt(
		"GetDecisionsByRequestID",
		// language=PostgreSQL
		`
SELECT d.*
FROM cms.decisions d
WHERE d.request_id = ANY(:ids_array)`,
		db_models.Decision{},
	)
	queryNotFinishedDecisionsByFQDN = sqlutil.Stmt{
		Name: "NotFinishedDecisionsByFQDN",
		// language=PostgreSQL
		Query: `
SELECT d.*
FROM cms.decisions d INNER JOIN cms.requests r ON d.request_id = r.id
WHERE r.fqdns = :fqdns AND r.is_deleted AND d.status NOT IN ('done', 'rejected')`,
	}
)

func (b *Backend) GetDecisionsToProcess(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error) {
	return b.getDecisionsByQuery(ctx, queryDecisionsToAnalyse, skipIDs)
}

func (b *Backend) getDecisionsByQuery(ctx context.Context, query sqlutil.Stmt, skipIDs []int64) (models.AutomaticDecision, error) {
	var dbDec db_models.Decision
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&dbDec)
	}

	args := map[string]interface{}{}
	if skipIDs == nil {
		skipIDs = make([]int64, 0)
	}
	var pgIds pgtype.Int8Array
	if err := pgIds.Set(skipIDs); err != nil {
		return models.AutomaticDecision{}, xerrors.Errorf("assign skip ids: %w", err)
	}
	args["skip_ids_array"] = pgIds
	c, err := sqlutil.QueryTx(
		ctx,
		query,
		args,
		parser,
		b.log,
	)
	if err != nil {
		return models.AutomaticDecision{}, err
	}
	if c == 0 {
		return models.AutomaticDecision{}, semerr.NotFound("no decisions to process")
	}
	return dbDec.ToInternal(b.log)
}

func (b *Backend) GetDecisionsToLetGo(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error) {
	return b.getDecisionsByQuery(ctx, queryDecisionsToLetGo, skipIDs)
}

func (b *Backend) GetDecisionsToReturnFromWalle(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error) {
	return b.getDecisionsByQuery(ctx, queryDecisionsToReturnFromWalle, skipIDs)
}

func (b *Backend) GetDecisionsToFinishAfterWalle(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error) {
	return b.getDecisionsByQuery(ctx, queryDecisionsToFinishAfterWalle, skipIDs)
}

func (b *Backend) GetDecisionsToCleanup(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error) {
	return b.getDecisionsByQuery(ctx, queryDecisionsToCleanup, skipIDs)
}

func (b *Backend) MoveDecisionsToStatus(ctx context.Context,
	decisionIDs []int64,
	status models.DecisionStatus) error {
	var pgIds pgtype.Int8Array
	if err := pgIds.Set(decisionIDs); err != nil {
		return err
	}
	_, err := sqlutil.QueryTx(
		ctx,
		queryMoveDecisionToStatus,
		map[string]interface{}{
			"decision_ids_array": pgIds,
			"status":             status,
		},
		sqlutil.NopParser,
		b.log,
	)
	return err
}

func (b *Backend) UpdateDecisionFields(ctx context.Context, d models.AutomaticDecision) error {
	dboml, err := db_models.OpsMetaLogToDB(*d.OpsLog)
	if err != nil {
		return err
	}
	_, err = sqlutil.QueryTx(
		ctx,
		queryUpdateDecisionFields,
		map[string]interface{}{
			"id":         d.ID,
			"expl":       d.AnalysisLog,
			"mutat":      d.MutationsLog,
			"aftlog":     d.AfterWalleLog,
			"cleanuplog": d.CleanupLog,
			"oplog":      string(dboml),
		},
		sqlutil.NopParser,
		b.log,
	)
	return err
}

func (b *Backend) SetAutoDutyResolution(ctx context.Context, decisionIDs []int64, res models.AutoResolution) error {
	var pgIds pgtype.Int8Array
	if err := pgIds.Set(decisionIDs); err != nil {
		return xerrors.Errorf("assign decisionIDs: %w", err)
	}
	_, err := sqlutil.QueryTx(
		ctx,
		querySetAutoDutyResolution,
		map[string]interface{}{
			"decision_ids_array": pgIds,
			"r":                  res,
		},
		sqlutil.NopParser,
		b.log,
	)
	return err
}

func (b *Backend) CreateDecision(ctx context.Context, requestID int64, status models.DecisionStatus, explanation string) (int64, error) {
	var decisionID int64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&decisionID)
	}

	if explanation == "" {
		if status == models.DecisionProcessing || status == models.DecisionNone {
			explanation = "to be processed"
		} else {
			return 0, xerrors.New("explanation should not be empty")
		}
	}
	dboml, err := db_models.OpsMetaLogToDB(models.NewOpsMetaLog())
	if err != nil {
		return 0, err
	}
	_, err = sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryInsertDecision,
		map[string]interface{}{
			"request_id":   requestID,
			"status":       status,
			"explanation":  explanation,
			"ops_metadata": string(dboml),
			"finish_log":   "",
			"cleanup_log":  "",
		},
		parser,
		b.log,
	)
	return decisionID, err
}

func (b *Backend) GetDecisionsByID(ctx context.Context, decisionIDs []int64) (map[int64]models.AutomaticDecision, error) {
	results := map[int64]models.AutomaticDecision{}
	parser := func(rows *sqlx.Rows) error {
		var d db_models.Decision
		if err := rows.StructScan(&d); err != nil {
			return err
		}
		r, err := d.ToInternal(b.log)
		if err != nil {
			return err
		}
		results[r.ID] = r
		return nil
	}

	var pgIds pgtype.Int8Array
	if err := pgIds.Set(decisionIDs); err != nil {
		return nil, err
	}
	args := map[string]interface{}{
		"decision_ids_array": pgIds,
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryDecisionsByID,
		args,
		parser,
		b.log,
	); err != nil {
		return nil, err
	}
	return results, nil
}

func (b *Backend) GetDecisionsByRequestID(ctx context.Context, requestIDs []int64) ([]models.AutomaticDecision, error) {
	var results []models.AutomaticDecision
	parser := func(rows *sqlx.Rows) error {
		var d db_models.Decision
		if err := rows.StructScan(&d); err != nil {
			return err
		}
		r, err := d.ToInternal(b.log)
		if err != nil {
			return err
		}
		results = append(results, r)
		return nil
	}

	var pgIds pgtype.Int8Array
	if err := pgIds.Set(requestIDs); err != nil {
		return nil, err
	}
	args := map[string]interface{}{
		"ids_array": pgIds,
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryDecisionsByRequestID,
		args,
		parser,
		b.log,
	); err != nil {
		return nil, err
	}
	return results, nil
}

func (b *Backend) GetNotFinishedDecisionsByFQDN(ctx context.Context, fqdns []string) ([]models.AutomaticDecision, error) {
	var results []models.AutomaticDecision
	parser := func(rows *sqlx.Rows) error {
		var d db_models.Decision
		if err := rows.StructScan(&d); err != nil {
			return err
		}
		r, err := d.ToInternal(b.log)
		if err != nil {
			return err
		}
		results = append(results, r)
		return nil
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryNotFinishedDecisionsByFQDN,
		map[string]interface{}{
			"fqdns": fqdns,
		},
		parser,
		b.log,
	); err != nil {
		return nil, err
	}

	return results, nil
}
