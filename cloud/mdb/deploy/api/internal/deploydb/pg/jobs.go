package pg

import (
	"context"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryCreateJob = sqlutil.Stmt{
		Name: "CreateJob",
		Query: "SELECT * FROM code.create_job(" +
			"i_ext_job_id => :ext_job_id, " +
			"i_command_id => :command_id)",
	}
	querySelectJob = sqlutil.Stmt{
		Name:  "SelectJob",
		Query: "SELECT * FROM code.get_job(i_job_id => :job_id)",
	}
	querySelectJobs = sqlutil.Stmt{
		Name: "SelectJobs",
		Query: "SELECT * FROM code.get_jobs(" +
			"i_shipment_id => :shipment_id, " +
			"i_fqdn => :fqdn, " +
			"i_ext_job_id => :ext_job_id, " +
			"i_status => :status, " +
			"i_limit => :limit, " +
			"i_last_job_id => :last_job_id, " +
			"i_ascending => :ascending)",
	}
	queryTimeoutJobs = sqlutil.Stmt{
		Name:  "TimeoutJobs",
		Query: "SELECT * FROM code.timeout_jobs(i_limit => :limit)",
	}
	querySelectRunningJobs = sqlutil.Stmt{
		Name:  "SelectRunningJobs",
		Query: "SELECT * FROM code.get_running_jobs_and_mark_them(i_master => :master, i_running => :running, i_limit => :limit)",
	}
	queryRunningJobMissing = sqlutil.Stmt{
		Name:  "RunningJobMissing",
		Query: "SELECT * FROM code.running_job_missing(i_ext_job_id => :ext_job_id, i_fqdn => :fqdn, i_fail_on_count => :fail_on_count)",
	}
)

func (b *backend) Job(ctx context.Context, id models.JobID) (models.Job, error) {
	return b.queryJob(ctx, b.cluster.AliveChooser(), querySelectJob, map[string]interface{}{"job_id": id})
}

func (b *backend) queryJob(ctx context.Context, chooser sqlutil.NodeChooser, stmt sqlutil.Stmt, args map[string]interface{}) (models.Job, error) {
	var job jobModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&job)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		chooser,
		stmt,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return models.Job{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.Job{}, semerr.NotFound("job not found")
	}

	return jobFromDB(job), nil
}

func (b *backend) Jobs(ctx context.Context, attrs deploydb.SelectJobsAttrs, limit int64, lastJobID optional.Int64) ([]models.Job, error) {
	args := map[string]interface{}{
		"limit":     limit,
		"ascending": sortOrderToDB(attrs.SortOrder),
	}
	if lastJobID.Valid {
		args["last_job_id"] = lastJobID.Must()
	} else {
		args["last_job_id"] = nil
	}

	if attrs.ShipmentID.Valid {
		args["shipment_id"] = attrs.ShipmentID.Int64
	} else {
		args["shipment_id"] = nil
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

	return b.queryJobsList(ctx, b.cluster.AliveChooser(), querySelectJobs, args)
}

func (b *backend) TimeoutJobs(ctx context.Context, limit int64) ([]models.Job, error) {
	return b.queryJobsList(ctx, b.cluster.PrimaryChooser(), queryTimeoutJobs, map[string]interface{}{"limit": limit})
}

func (b *backend) queryJobsList(ctx context.Context, chooser sqlutil.NodeChooser, stmt sqlutil.Stmt, args map[string]interface{}) ([]models.Job, error) {
	var jobs []models.Job
	parser := func(rows *sqlx.Rows) error {
		var job jobModel
		if err := rows.StructScan(&job); err != nil {
			return err
		}

		jobs = append(jobs, jobFromDB(job))
		return nil
	}

	_, err := sqlutil.QueryContext(
		ctx,
		chooser,
		stmt,
		args,
		parser,
		b.logger,
	)
	return jobs, errorToSemErr(err)
}

func (b *backend) RunningJobsForCheck(ctx context.Context, master string, running time.Duration, limit int64) (deploydb.RunningJobsChecker, error) {
	d := &pgtype.Interval{}
	if err := d.Set(running); err != nil {
		return nil, xerrors.Errorf("failed to set running argument for %s query: %w", querySelectRunningJobs.Name, err)
	}

	checker := runningJobsChecker{lg: b.logger, b: b}
	parser := func(rows *sqlx.Rows) error {
		var job struct {
			ExtJobID string `db:"ext_job_id"`
			Minion   string `db:"minion"`
		}
		if err := rows.StructScan(&job); err != nil {
			return err
		}

		checker.jobs = append(checker.jobs, deploydb.RunningJob(job))
		return nil
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		querySelectRunningJobs,
		map[string]interface{}{
			"master":  master,
			"running": d,
			"limit":   limit,
		},
		parser,
		b.logger,
	); err != nil {
		return nil, errorToSemErr(err)
	}

	return &checker, nil
}

type runningJobsChecker struct {
	lg   log.Logger
	jobs []deploydb.RunningJob
	b    *backend
}

var _ deploydb.RunningJobsChecker = &runningJobsChecker{}

func (r *runningJobsChecker) Jobs() []deploydb.RunningJob {
	return r.jobs
}

func (r *runningJobsChecker) NotRunning(ctx context.Context, job deploydb.RunningJob, failOnCount int) (models.Job, error) {
	return r.b.queryJob(
		ctx,
		r.b.cluster.PrimaryChooser(),
		queryRunningJobMissing,
		map[string]interface{}{
			"ext_job_id":    job.ExtJobID,
			"fqdn":          job.Minion,
			"fail_on_count": failOnCount,
		},
	)
}
