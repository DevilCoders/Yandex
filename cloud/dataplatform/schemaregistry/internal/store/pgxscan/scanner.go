package pgxscan

import (
	"context"
	"errors"

	"github.com/jackc/pgx/v4"
	"github.com/jackc/pgx/v4/pgxpool"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/store/dbscan"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Querier is something that pgxscan can query and get the pgx.Rows from.
// For example, it can be: *pgxpool.Pool, *pgx.Conn or pgx.Tx.
type Querier interface {
	Query(ctx context.Context, query string, args ...interface{}) (pgx.Rows, error)
}

var (
	_ Querier = &pgxpool.Pool{}
	_ Querier = &pgx.Conn{}
	_ Querier = pgx.Tx(nil)
)

// Select is a high-level function that queries rows from Querier and calls the ScanAll function.
// See ScanAll for details.
func Select(ctx context.Context, db Querier, dst interface{}, query string, args ...interface{}) error {
	rows, err := db.Query(ctx, query, args...)
	if err != nil {
		return xerrors.Errorf("scany: query multiple result rows: %w", err)
	}
	return ScanAll(dst, rows)
}

// Get is a high-level function that queries rows from Querier and calls the ScanOne function.
// See ScanOne for details.
func Get(ctx context.Context, db Querier, dst interface{}, query string, args ...interface{}) error {
	rows, err := db.Query(ctx, query, args...)
	if err != nil {
		return xerrors.Errorf("scany: query one result row: %w", err)
	}
	return ScanOne(dst, rows)
}

// ScanAll is a wrapper around the dbscan.ScanAll function.
// See dbscan.ScanAll for details.
func ScanAll(dst interface{}, rows pgx.Rows) error {
	return dbscan.ScanAll(dst, NewRowsAdapter(rows))
}

// ScanOne is a wrapper around the dbscan.ScanOne function.
// See dbscan.ScanOne for details. If no rows are found it
// returns a pgx.ErrNoRows error.
func ScanOne(dst interface{}, rows pgx.Rows) error {
	err := dbscan.ScanOne(dst, NewRowsAdapter(rows))
	if dbscan.NotFound(err) {
		return pgx.ErrNoRows
	}
	return err
}

// NotFound is a helper function to check if an error
// is `pgx.ErrNoRows`.
func NotFound(err error) bool {
	return errors.Is(err, pgx.ErrNoRows)
}

// RowScanner is a wrapper around the dbscan.RowScanner type.
// See dbscan.RowScanner for details.
type RowScanner struct {
	*dbscan.RowScanner
}

// NewRowScanner returns a new RowScanner instance.
func NewRowScanner(rows pgx.Rows) *RowScanner {
	ra := NewRowsAdapter(rows)
	return &RowScanner{
		RowScanner: dbscan.NewRowScanner(ra),
	}
}

// ScanRow is a wrapper around the dbscan.ScanRow function.
// See dbscan.ScanRow for details.
func ScanRow(dst interface{}, rows pgx.Rows) error {
	return dbscan.ScanRow(dst, NewRowsAdapter(rows))
}

// RowsAdapter makes pgx.Rows compliant with the dbscan.Rows interface.
// See dbscan.Rows for details.
type RowsAdapter struct {
	pgx.Rows
}

// NewRowsAdapter returns a new RowsAdapter instance.
func NewRowsAdapter(rows pgx.Rows) *RowsAdapter {
	return &RowsAdapter{Rows: rows}
}

// Columns implements the dbscan.Rows.Columns method.
func (ra RowsAdapter) Columns() ([]string, error) {
	columns := make([]string, len(ra.Rows.FieldDescriptions()))
	for i, fd := range ra.Rows.FieldDescriptions() {
		columns[i] = string(fd.Name)
	}
	return columns, nil
}

// Close implements the dbscan.Rows.Close method.
func (ra RowsAdapter) Close() error {
	ra.Rows.Close()
	return nil
}
