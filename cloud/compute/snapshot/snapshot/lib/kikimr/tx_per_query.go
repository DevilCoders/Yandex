package kikimr

import (
	"context"
	"database/sql"
	"runtime"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"

	"golang.org/x/xerrors"
)

type sqlTxPerQuery struct {
	db     kikimrDB
	txOpts *sql.TxOptions
}
type errorScanner struct {
	err error
}

func (e errorScanner) Err() error {
	return e.err
}

func (e errorScanner) Scan(args ...interface{}) error {
	return e.err
}

func (s sqlTxPerQuery) ExecContext(ctx context.Context, query string, args ...interface{}) (res sql.Result, resErr error) {
	resErr = s.retryQueryInTx(ctx, true, func(ctx context.Context, tx kikimrTx) error {
		res, resErr = tx.ExecContext(ctx, query, args...)
		return resErr
	})
	return
}

func (s sqlTxPerQuery) QueryContext(ctx context.Context, query string, args ...interface{}) (res SQLRows, resErr error) {
	resErr = s.retryQueryInTx(ctx, false, func(ctx context.Context, tx kikimrTx) error {
		log.G(ctx).Info("QueryContext inner")
		res, resErr = tx.QueryContext(ctx, query, args...)
		res = newSQLTxPerQueryRows(tx, res)
		return resErr
	})

	return
}

func (s sqlTxPerQuery) QueryRowContext(ctx context.Context, query string, args ...interface{}) (res SQLRow) {
	resErr := s.retryQueryInTx(ctx, false, func(ctx context.Context, tx kikimrTx) error {
		res = tx.QueryRowContext(ctx, query, args...)
		return res.Err()
	})
	if res == nil {
		res = errorScanner{err: resErr}
	}

	return
}

func (s sqlTxPerQuery) retryQueryInTx(ctx context.Context, commitTransaction bool, f func(ctx context.Context, tx kikimrTx) error) (resErr error) {
	_ = misc.Retry(ctx, "short-transaction", func() (retryErr error) {
		log.G(ctx).Info("retry inner")
		var tx kikimrTx
		tx, resErr = s.db.BeginTx(ctx, s.txOpts)
		if resErr != nil {
			log.G(ctx).Error("Error start short transaction", zap.Error(resErr))
			resErr = xerrors.Errorf("start short transaction: %w", resErr)
			return convertRetryableError(resErr)
		}

		defer func() {
			if resErr == nil {
				if commitTransaction {
					resErr = tx.Commit()
					if resErr != nil {
						log.G(ctx).Error("Error commit short transaction", zap.Error(resErr))
						resErr = xerrors.Errorf("commit short transaction: %w", resErr)
					}
				}
				retryErr = convertRetryableError(resErr)
			}
		}()

		resErr = f(ctx, tx)
		if resErr != nil {
			log.G(ctx).Error("Error query in short transaction", zap.Error(resErr))
		}
		return convertRetryableError(resErr)
	})
	return
}

type sqlTxPerQueryRow struct {
	err error
	tx  kikimrTx
	row SQLRow
}

func newSQLTxPerQueryRow(tx kikimrTx, row SQLRow) *sqlTxPerQueryRow {
	res := &sqlTxPerQueryRow{
		tx:  tx,
		row: row,
	}

	// if rows unreachable - need to close transaction
	runtime.SetFinalizer(res, func(o *sqlTxPerQueryRow) {
		go func() {
			_ = o.tx.Commit()
		}()
	})
	return res
}

func (s *sqlTxPerQueryRow) Scan(dest ...interface{}) error {
	err := s.row.Scan(dest...)
	runtime.SetFinalizer(s, nil)
	commitErr := s.tx.Commit()
	if err == nil {
		err = commitErr
	}
	return err
}

func (s *sqlTxPerQueryRow) Err() error {
	err := s.row.Err()
	if err == nil {
		err = s.err
	}
	return err
}

type sqlTxPerQueryRows struct {
	err  error
	tx   kikimrTx
	rows SQLRows
}

func newSQLTxPerQueryRows(tx kikimrTx, rows SQLRows) *sqlTxPerQueryRows {
	res := &sqlTxPerQueryRows{
		tx:   tx,
		rows: rows,
	}

	// if rows unreachable - need to close transaction
	runtime.SetFinalizer(res, func(o *sqlTxPerQueryRows) {
		go func() {
			_ = o.Close()
		}()
	})
	return res
}

func (s *sqlTxPerQueryRows) Close() error {
	runtime.SetFinalizer(s, nil)

	defer func() {
		_ = s.tx.Commit()
	}()
	return s.rows.Close()
}

func (s *sqlTxPerQueryRows) Next() bool {
	res := s.rows.Next()
	if !res {
		// close transaction when read all lines
		runtime.SetFinalizer(s, nil)
		s.err = s.tx.Commit()
	}
	return res
}

func (s *sqlTxPerQueryRows) Scan(dest ...interface{}) error {
	return s.rows.Scan(dest...)
}

func (s *sqlTxPerQueryRows) Err() error {
	err := s.rows.Err()
	if err == nil {
		err = s.err
	}
	return err
}
