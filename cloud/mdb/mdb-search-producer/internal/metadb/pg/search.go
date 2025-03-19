package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/metadb"
	pgxutil "a.yandex-team.ru/library/go/x/sql/pgx"
)

var (
	queryEnumerateSearchQueue = sqlutil.Stmt{
		Name: "EnumerateSearchQueue",
		// language=PostgreSQL
		Query: `UPDATE dbaas.search_queue
                   SET queue_id = new_queue_id
				  FROM (SELECT sq_id,
							   nextval('dbaas.search_queue_queue_ids') AS new_queue_id
						  FROM dbaas.search_queue
						 WHERE queue_id IS NULL
					     ORDER BY created_at
                         LIMIT 100) nq
				 WHERE nq.sq_id = search_queue.sq_id`,
	}

	querySelectNonEnumeratedSearchQueue = sqlutil.NewStmt(
		"SelectNonEnumeratedSearchQueue",
		// language=PostgreSQL
		`SELECT * FROM dbaas.search_queue WHERE queue_id IS NULL ORDER BY created_at LIMIT :limit`,
		nonEnumeratedRow{},
	)

	querySelectUnsentSearchQueue = sqlutil.NewStmt(
		"SelectUnsentSearchQueue",
		// language=PostgreSQL
		`SELECT *
		   FROM dbaas.search_queue
		  WHERE sent_at IS NULL
		    AND queue_id IS NOT NULL
		  ORDER BY queue_id
		  LIMIT :limit`,
		unsentRow{})

	queryMarkSearchDocsAsSent = sqlutil.Stmt{
		Name: "MarkSearchDocsAsSent",
		// language=PostgreSQL
		Query: `UPDATE dbaas.search_queue SET sent_at = now() WHERE queue_id = ANY(:sent_ids)`,
	}

	queryTrySearchLock = sqlutil.Stmt{
		Name: "TrySearchLock",
		// language=PostgreSQL
		Query: "SELECT pg_try_advisory_xact_lock(1000)",
	}
)

func (mdb *metaDB) NonEnumeratedSearchDocs(ctx context.Context, limit int64) ([]metadb.NonEnumeratedSearchDoc, error) {
	var ret []metadb.NonEnumeratedSearchDoc
	parser := func(rows *sqlx.Rows) error {
		var r nonEnumeratedRow
		if err := rows.StructScan(&r); err != nil {
			return err
		}
		ret = append(ret, r.format())
		return nil
	}

	_, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		querySelectNonEnumeratedSearchQueue,
		map[string]interface{}{
			"limit": limit,
		},
		parser,
		mdb.logger,
	)
	if err != nil {
		return nil, err
	}
	return ret, nil
}

func (mdb *metaDB) UnsentSearchDocs(ctx context.Context, limit int64) ([]metadb.UnsentSearchDoc, error) {
	var parser unsetRowsParser

	if _, err := sqlutil.QueryContext(
		ctx,
		mdb.cluster.AliveChooser(),
		querySelectUnsentSearchQueue,
		map[string]interface{}{"limit": limit},
		parser.parse,
		mdb.logger,
	); err != nil {
		return nil, err
	}

	return parser.ret, nil
}

func (mdb *metaDB) OnUnsentSearchQueueDoc(ctx context.Context, limit int64, handler metadb.OnSearchQueueEventsHandler) (int, error) {
	var documentsSent int

	err := sqlutil.InTxDoQueries(ctx, mdb.cluster.Primary(), mdb.logger, func(query sqlutil.QueryCallback) error {
		var gotLock bool

		if err := query(
			queryTrySearchLock,
			map[string]interface{}{},
			func(rows *sqlx.Rows) error {
				return rows.Scan(&gotLock)
			},
		); err != nil {
			return err
		}

		if !gotLock {
			mdb.logger.Debug("I don't have a lock")
			return nil
		}

		if err := query(
			queryEnumerateSearchQueue,
			map[string]interface{}{},
			sqlutil.NopParser,
		); err != nil {
			return err
		}

		unsentParser := unsetRowsParser{}

		if err := query(
			querySelectUnsentSearchQueue,
			map[string]interface{}{"limit": limit},
			unsentParser.parse,
		); err != nil {
			return err
		}

		if len(unsentParser.ret) == 0 {
			mdb.logger.Debug("there are no unsent documents")
			return nil
		}

		sentEventsIDs, err := handler(ctx, unsentParser.ret)
		if err != nil {
			return err
		}
		documentsSent = len(sentEventsIDs)
		if documentsSent == 0 {
			mdb.logger.Infof("handler don't sent anything")
			return nil
		}

		return query(
			queryMarkSearchDocsAsSent,
			map[string]interface{}{
				"sent_ids": pgxutil.Array(sentEventsIDs),
			},
			sqlutil.NopParser,
		)
	})
	return documentsSent, err
}
