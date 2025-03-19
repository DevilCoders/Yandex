package mysqlutil

import (
	"context"
	"fmt"

	"github.com/jmoiron/sqlx"
	gtids "github.com/siddontang/go-mysql/mysql"
)

// Ping runs simple query to check server stauts
func Ping(ctx context.Context, db *sqlx.DB) error {
	row := db.QueryRowContext(ctx, "SELECT 1")
	return row.Scan(new(int))
}

// GetSlaveStatus returns SLAVE STATUS or nil mysql is master
func GetSlaveStatus(ctx context.Context, db *sqlx.DB) (*SlaveStatus, error) {
	rows, err := db.QueryxContext(ctx, "SHOW SLAVE STATUS")
	if err != nil {
		return nil, err
	}
	defer func() { _ = rows.Close() }()
	if !rows.Next() {
		return nil, nil
	}
	result := new(SlaveStatus)
	err = rows.StructScan(result)
	if err != nil {
		return nil, err
	}
	return result, err
}

// get ReplicationLag from mdb_repl_mon
func GetMdbReplicationLag(ctx context.Context, db *sqlx.DB) (float64, error) {
	var lagSeconds float64
	err := db.GetContext(ctx, &lagSeconds, "SELECT CURRENT_TIMESTAMP(3) - ts FROM mysql.mdb_repl_mon")
	if err != nil {
		return 0, err
	}
	return lagSeconds, nil
}

// GTIDExecuted returns global transaction id executed
func GetGTIDExecuted(ctx context.Context, db *sqlx.DB) (gtids.GTIDSet, error) {
	rows, err := db.QueryxContext(ctx, "SELECT @@GLOBAL.gtid_executed  as Executed_Gtid_Set")
	if err != nil {
		return nil, err
	}
	defer func() { _ = rows.Close() }()
	if !rows.Next() {
		return nil, fmt.Errorf("failed to fetch gtid_executed - empty result")
	}
	gtidset := new(GTIDExecuted)
	err = rows.StructScan(gtidset)
	if err != nil {
		return nil, err
	}
	parsed, err := gtids.ParseGTIDSet(gtids.MySQLFlavor, gtidset.ExecutedGtidSet)
	if err != nil {
		return nil, err
	}
	return parsed, nil
}

// GetBinlogs returns SHOW BINARY LOGS output
func GetBinlogs(ctx context.Context, db *sqlx.DB) ([]Binlog, error) {
	rows, err := db.QueryxContext(ctx, "SHOW BINARY LOGS")
	if err != nil {
		return nil, err
	}
	defer func() { _ = rows.Close() }()
	var binlogs []Binlog
	for rows.Next() {
		var b Binlog
		err = rows.StructScan(&b)
		if err != nil {
			return nil, err
		}
		binlogs = append(binlogs, b)
	}
	return binlogs, nil
}

// IsOffline returns whether mysql offline_mode is ON
func IsOffline(ctx context.Context, db *sqlx.DB) (bool, error) {
	var offlineMode int
	err := db.GetContext(ctx, &offlineMode, "SELECT @@GLOBAL.offline_mode")
	if err != nil {
		return false, err
	}
	return offlineMode != 0, nil
}
