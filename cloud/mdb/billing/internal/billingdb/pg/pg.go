package pg

import (
	"context"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var _ billingdb.BillingDB = &billingDB{}

type LockKey int64

var (
	Backup       LockKey = 1000
	CloudStorage LockKey = 2000
)

var btypeToLockKey = map[billingdb.BillType]LockKey{
	billingdb.BillTypeBackup:       Backup,
	billingdb.BillTypeCloudStorage: CloudStorage,
}

var (
	querySelectTracks = sqlutil.Stmt{
		Name: "SelectClusterTracks",
		// language=PostgreSQL
		Query: `
SELECT
	t.cluster_id,
	t.cluster_type,
	t.bill_type,
	t.from_ts,
	t.until_ts
FROM
	billing.tracks t
WHERE
	t.bill_type = :bill_type
FOR UPDATE
SKIP LOCKED
`,
	}

	queryUpdateClusterTrack = sqlutil.Stmt{
		Name: "UpdateClusterTrack",
		// language=PostgreSQL
		Query: `
UPDATE billing.tracks
  SET until_ts = :until_ts,
      updated_at = now()
WHERE
    cluster_id = :cluster_id AND
    bill_type = :bill_type
RETURNING 1
`,
	}

	queryEnqueueMetricsBatch = sqlutil.Stmt{
		Name: "EnqueueMetricsBatch",
		Query:
		// language=PostgreSQL
		`
INSERT INTO billing.metrics_queue
	(batch_id, bill_type, batch, seq_no)
VALUES
    (:batch_id, :bill_type, :batch, (SELECT COALESCE(max(seq_no), 0) + 1 FROM billing.metrics_queue WHERE bill_type = :bill_type))
`,
	}

	querySelectOldestPendingBatchByType = sqlutil.Stmt{
		Name: "SelectOldestPendingBatchByType",
		// language=PostgreSQL
		Query: `
SELECT
    batch_id,
	created_at,
	restart_count,
	seq_no,
	batch
FROM billing.metrics_queue
WHERE bill_type = :bill_type
  AND finished_at IS NULL
ORDER BY seq_no
LIMIT 1
FOR UPDATE
`,
	}

	queryPostponeMetricsBatch = sqlutil.Stmt{
		Name: "PostponeMetricsBatch",
		// language=PostgreSQL
		Query: `
UPDATE billing.metrics_queue
  SET updated_at = now(),
      finished_at = NULL,
  	  restart_count = restart_count + 1
WHERE
    batch_id = :batch_id
RETURNING 1
`,
	}

	queryCompleteMetricsBatch = sqlutil.Stmt{
		Name: "CompleteMetricsBatch",
		// language=PostgreSQL
		Query: `
UPDATE billing.metrics_queue
  SET started_at = :started_at,
      finished_at = :finished_at,
      updated_at = now()
WHERE
    batch_id = :batch_id
RETURNING 1
`,
	}

	queryTryGetLock = sqlutil.Stmt{
		Name: "TryGetLock",
		// language=PostgreSQL
		Query: `
SELECT pg_try_advisory_xact_lock(:key)
`,
	}
)

// billingDB implements BillingDB interface for PostgreSQL
type billingDB struct {
	logger  log.Logger
	cluster *sqlutil.Cluster
}

// New constructs billingDB
func New(cfg pgutil.Config, l log.Logger) (billingdb.BillingDB, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}

	return &billingDB{logger: l, cluster: cluster}, nil
}

func (bdb *billingDB) Begin(ctx context.Context, ns sqlutil.NodeStateCriteria) (context.Context, error) {
	binding, err := sqlutil.Begin(ctx, bdb.cluster, ns, nil)
	if err != nil {
		return ctx, err
	}

	return sqlutil.WithTxBinding(ctx, binding), nil
}

func (bdb *billingDB) Commit(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Commit(ctx)
}

func (bdb *billingDB) Rollback(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Rollback(ctx)
}

func (bdb *billingDB) TryGetLock(ctx context.Context, btype billingdb.BillType) error {
	key, ok := btypeToLockKey[btype]
	if !ok {
		return xerrors.Errorf("lock key for btype=%s not exists", btype)
	}

	var gotLock bool
	if _, err := sqlutil.QueryTx(
		ctx,
		queryTryGetLock,
		map[string]interface{}{
			"key": key,
		},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&gotLock)
		},
		bdb.logger,
	); err != nil {
		return err
	}
	if !gotLock {
		bdb.logger.Errorf("Lock not taken for btype = %s, key = %d", btype, key)
		return billingdb.ErrLockNotTaken
	}
	bdb.logger.Debugf("Lock taken for btype = %s, key = %d", btype, key)
	return nil
}

func (bdb *billingDB) Tracks(ctx context.Context, billType billingdb.BillType) ([]billingdb.Track, error) {
	var ts []billingdb.Track
	parser := func(rows *sqlx.Rows) error {

		var row trackRow
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		ts = append(ts, trackFromDB(row))
		return nil
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectTracks,
		map[string]interface{}{
			"bill_type": billType,
		},
		parser,
		bdb.logger,
	)
	if err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, billingdb.ErrDataNotFound
	}

	return ts, nil
}

func (bdb *billingDB) UpdateClusterTrack(ctx context.Context, clusterID string, untilTS time.Time, billType billingdb.BillType) error {
	count, err := sqlutil.QueryTx(
		ctx,
		queryUpdateClusterTrack,
		map[string]interface{}{
			"cluster_id": clusterID,
			"bill_type":  billType,
			"until_ts":   untilTS,
		},
		sqlutil.NopParser,
		bdb.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return billingdb.ErrDataNotFound
	}
	return nil
}

func (bdb *billingDB) EnqueueMetrics(ctx context.Context, metrics billingdb.Metrics, billType billingdb.BillType) error {
	metricsbytes, err := metrics.Marshal()
	if err != nil {
		return err
	}

	_, err = sqlutil.QueryTx(
		ctx,
		queryEnqueueMetricsBatch,
		map[string]interface{}{
			"batch_id":  metrics.ID(),
			"bill_type": billType,
			"batch":     string(metricsbytes),
		},
		sqlutil.NopParser,
		bdb.logger,
	)

	return err
}

func (bdb *billingDB) DequeueBatch(ctx context.Context, billType billingdb.BillType) (billingdb.Batch, error) {
	var row metricsBatchRow
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&row)
	}
	count, err := sqlutil.QueryTx(
		ctx,
		querySelectOldestPendingBatchByType,
		map[string]interface{}{
			"bill_type": billType,
		},
		parser,
		bdb.logger,
	)
	if err != nil {
		return billingdb.Batch{}, err
	}
	if count == 0 {
		return billingdb.Batch{}, billingdb.ErrDataNotFound
	}
	return metricsBatchFromDB(row), nil
}

func (bdb *billingDB) CompleteBatch(ctx context.Context, batchID string, startedTS, finishedTS time.Time) error {
	count, err := sqlutil.QueryTx(
		ctx,
		queryCompleteMetricsBatch,
		map[string]interface{}{
			"batch_id":    batchID,
			"started_at":  startedTS,
			"finished_at": finishedTS,
		},
		sqlutil.NopParser,
		bdb.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return billingdb.ErrDataNotFound
	}
	return nil
}

func (bdb *billingDB) PostponeBatch(ctx context.Context, batchID string) error {
	count, err := sqlutil.QueryTx(
		ctx,
		queryPostponeMetricsBatch,
		map[string]interface{}{
			"batch_id": batchID,
		},
		sqlutil.NopParser,
		bdb.logger,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return billingdb.ErrDataNotFound
	}
	return nil
}

func (bdb *billingDB) Close() error {
	return bdb.cluster.Close()
}

func (bdb *billingDB) IsReady(context.Context) error {
	if bdb.cluster.Primary() == nil {
		return semerr.Unavailable("unavailable")
	}

	return nil
}
