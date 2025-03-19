package pg

import (
	"context"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryCleanupUnboundJobResults = sqlutil.Stmt{
		Name:  "CleanupUnboundJobResults",
		Query: "SELECT * FROM code.cleanup_unbound_job_results(i_age => :age, i_limit => :limit)",
	}
	queryCleanupShipments = sqlutil.Stmt{
		Name:  "CleanupShipment",
		Query: "SELECT * FROM code.cleanup_shipments(i_age => :age, i_limit => :limit)",
	}
	queryCleanupMinionsWithoutJobs = sqlutil.Stmt{
		Name:  "CleanupMinionsWithoutJobs",
		Query: "SELECT * FROM code.cleanup_minions_without_jobs(i_age => :age, i_limit => :limit)",
	}
)

func (b *backend) cleanupQuery(ctx context.Context, stm sqlutil.Stmt, age time.Duration, limit uint64) (uint64, error) {
	var count uint64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&count)
	}
	pgAge := &pgtype.Interval{}
	if err := pgAge.Set(age); err != nil {
		return 0, xerrors.Errorf("error while converting timeout for statement %s with msq %w", stm.Name, err)
	}

	_, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		stm,
		map[string]interface{}{"age": pgAge, "limit": limit},
		parser,
		b.logger)

	if err != nil {
		return 0, errorToSemErr(err)
	}

	return count, nil
}

func (b *backend) CleanupUnboundJobResults(ctx context.Context, age time.Duration, limit uint64) (uint64, error) {
	return b.cleanupQuery(ctx, queryCleanupUnboundJobResults, age, limit)
}

func (b *backend) CleanupShipments(ctx context.Context, age time.Duration, limit uint64) (uint64, error) {
	return b.cleanupQuery(ctx, queryCleanupShipments, age, limit)
}

func (b *backend) CleanupMinionsWithoutJobs(ctx context.Context, age time.Duration, limit uint64) (uint64, error) {
	return b.cleanupQuery(ctx, queryCleanupMinionsWithoutJobs, age, limit)
}
