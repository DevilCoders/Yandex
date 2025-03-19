package pg

import (
	"context"
	"database/sql"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

var (
	queryCreateJobResult = sqlutil.Stmt{
		Name: "CreateJobResult",
		Query: "SELECT * " +
			"FROM code.create_job_result(" +
			"i_ext_job_id => :ext_job_id, " +
			"i_fqdn => :fqdn, " +
			"i_status => :status, " +
			"i_result => :result) ",
	}

	querySelectJobResult = sqlutil.Stmt{
		Name:  "SelectJobResult",
		Query: "SELECT * FROM code.get_job_result(i_job_result_id => :job_result_id)",
	}

	querySelectJobResults = sqlutil.Stmt{
		Name: "SelectJobResults",
		Query: "SELECT * FROM code.get_job_results(" +
			"i_ext_job_id => :ext_job_id, " +
			"i_fqdn => :fqdn, " +
			"i_status => :status, " +
			"i_limit => :limit, " +
			"i_last_job_result_id => :last_job_result_id, " +
			"i_ascending => :ascending)",
	}

	querySelectJobResultCoords = sqlutil.Stmt{
		Name: "SelectJobResultCoords",
		Query: "SELECT * FROM code.get_job_result_coords(" +
			"i_ext_job_id => :ext_job_id, " +
			"i_fqdn => :fqdn)",
	}
)

func (b *backend) CreateJobResult(ctx context.Context, id, fqdn string, status models.JobResultStatus, result []byte) (models.JobResult, error) {
	var jobresult jobResultModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&jobresult)
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryCreateJobResult,
		map[string]interface{}{
			"ext_job_id": id,
			"fqdn":       fqdn,
			"status":     status,
			"result":     string(result), // TODO: pgtype.JSON?
		},
		parser,
		b.logger,
	); err != nil {
		return models.JobResult{}, errorToSemErr(err)
	}

	return jobResultFromDB(jobresult), nil
}

func (b *backend) JobResult(ctx context.Context, id models.JobResultID) (models.JobResult, error) {
	var jr jobResultModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&jr)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectJobResult,
		map[string]interface{}{"job_result_id": id},
		parser,
		b.logger,
	)
	if err != nil {
		return models.JobResult{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.JobResult{}, semerr.NotFoundf("job result %d not found", id)
	}

	return jobResultFromDB(jr), nil
}

func (b *backend) JobResults(ctx context.Context, attrs deploydb.SelectJobResultsAttrs, limit int64, lastJobResultID optional.Int64) ([]models.JobResult, error) {
	var jobresults []models.JobResult
	parser := func(rows *sqlx.Rows) error {
		var jobresult jobResultModel
		if err := rows.StructScan(&jobresult); err != nil {
			return err
		}

		jobresults = append(jobresults, jobResultFromDB(jobresult))
		return nil
	}

	args := map[string]interface{}{
		"limit":     limit,
		"ascending": sortOrderToDB(attrs.SortOrder),
	}
	if lastJobResultID.Valid {
		args["last_job_result_id"] = lastJobResultID.Must()
	} else {
		args["last_job_result_id"] = nil
	}

	if attrs.ExtJobID.Valid {
		args["ext_job_id"] = attrs.ExtJobID.String
	} else {
		args["ext_job_id"] = nil
	}

	if attrs.FQDN.Valid {
		args["fqdn"] = attrs.FQDN.String
	} else {
		args["fqdn"] = nil
	}

	if attrs.Status.Valid {
		args["status"] = attrs.Status.String
	} else {
		args["status"] = nil
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectJobResults,
		args,
		parser,
		b.logger,
	); err != nil {
		return nil, errorToSemErr(err)
	}

	return jobresults, nil
}

func (b *backend) JobResultCoords(ctx context.Context, jobExtID, fqdn string) (models.JobResultCoords, error) {
	var jrc struct {
		ShipmentID models.ShipmentID `db:"shipment_id"`
		CommandID  models.CommandID  `db:"command_id"`
		JobID      models.JobID      `db:"job_id"`
		Tracing    sql.NullString    `db:"tracing"`
	}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&jrc)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectJobResultCoords,
		map[string]interface{}{
			"ext_job_id": jobExtID,
			"fqdn":       fqdn,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return models.JobResultCoords{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.JobResultCoords{}, semerr.NotFoundf("job result for jid %q and fqdn %q not found", jobExtID, fqdn)
	}

	return jobResultCoordsFromDB(jrc)
}
