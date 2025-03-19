package qtool

import (
	"io"

	"github.com/jmoiron/sqlx"
)

func NextResultSet(r *sqlx.Rows) (*sqlx.Rows, error) {
	if !r.NextResultSet() {
		return r, io.ErrUnexpectedEOF
	}
	return &sqlx.Rows{Rows: r.Rows, Mapper: r.Mapper}, nil
}
