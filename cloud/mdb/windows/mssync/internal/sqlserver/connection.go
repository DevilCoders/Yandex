package sqlserver

import (
	"database/sql"

	_ "github.com/denisenkom/go-mssqldb"
)

func GetSQLServerConnection(ConnectionString string) (*sql.DB, error) {
	db, err := sql.Open("sqlserver", ConnectionString)
	if err != nil {
		return nil, err
	}
	err = db.Ping()
	if err != nil {
		return nil, err
	}
	return db, nil
}
