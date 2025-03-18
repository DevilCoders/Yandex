package pgx

import (
	"database/sql"
	"database/sql/driver"

	"github.com/jackc/pgtype"
)

// JB is a generic JSONB struct
type JB struct {
	V interface{}
}

// JSONB is a helper function for Postgres jsonb values manupulations.
// It implements basic database/sql interfaces.
// Example:
//  type J struct {
//    id int `json:"id"`
//  }
//
//  db.Query(`SELECT * FROM t WHERE json_data = $1`, pgxutil.JSONB(J{42}))
//
//  var j J
//  db.QueryRow('SELECT json_data').Scan(pgxutil.JSONB(&j))
//
func JSONB(v interface{}) interface {
	driver.Valuer
	sql.Scanner
} {
	return JB{v}
}

// Value implements the sql.Valuer interface.
func (jsonb JB) Value() (driver.Value, error) {
	jb := new(pgtype.JSONB)
	if err := jb.Set(jsonb.V); err != nil {
		return nil, err
	}
	return jb, nil
}

// Scan implements the sql.Scanner interface.
func (jsonb JB) Scan(src interface{}) error {
	var jb pgtype.JSONB
	if err := jb.Scan(src); err != nil {
		return err
	}
	return jb.AssignTo(jsonb.V)
}
