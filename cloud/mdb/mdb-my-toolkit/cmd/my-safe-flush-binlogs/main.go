package main

import (
	"context"
	"log"
	"os"
	"time"

	"github.com/jmoiron/sqlx"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/mysqlutil"
)

var timeout time.Duration
var flushInterval time.Duration
var defaultsFile string

func init() {
	pflag.DurationVarP(&timeout, "timeout", "t", 15*time.Second, "timeout for all operations")
	pflag.DurationVarP(&flushInterval, "flushInterval", "i", 1*time.Hour, "how often we should flush binlogs")
	pflag.StringVar(&defaultsFile, "defaults-file", "", "mysql client settings file")
}

func getLastFlushTime(ctx context.Context, db *sqlx.DB) time.Time {
	var binlogIndexPath string
	row := db.QueryRowContext(ctx, "SELECT @@log_bin_index AS BinIndex")
	err := row.Scan(&binlogIndexPath)
	if err != nil {
		log.Fatalf("failed to get path to log_bin_index: %v", err)
	}
	file, err := os.Stat(binlogIndexPath)
	if err != nil {
		log.Fatalf("failed to stat %s: %v", binlogIndexPath, err)
	}
	return file.ModTime()
}

func ableToCommit(ctx context.Context, db *sqlx.DB) bool {
	var gtid1, gtid2 string
	query := "SELECT @@global.gtid_executed"
	row := db.QueryRowContext(ctx, query)
	err := row.Scan(&gtid1)
	if err != nil {
		log.Printf("query error %s: %v", query, err)
		return false
	}
	time.Sleep(timeout / 3)
	row = db.QueryRowContext(ctx, query)
	err = row.Scan(&gtid2)
	if err != nil {
		log.Printf("query error %s: %v", query, err)
		return false
	}
	return gtid2 != gtid1
}

func flushBinlogs(ctx context.Context, db *sqlx.DB) {
	_, err := db.ExecContext(ctx, "FLUSH LOCAL BINARY LOGS")
	if err != nil {
		log.Fatalf("failed to flush binlogs: %v", err)
	}
	log.Printf("done")
}

func main() {
	pflag.Parse()
	ctx := context.Background()
	ctx = signals.WithCancelOnSignal(ctx)
	ctx, cancel := context.WithTimeout(ctx, timeout)
	defer cancel()

	db, err := mysqlutil.ConnectWithDefaultsFile(defaultsFile)
	if err != nil {
		log.Fatalf("failed to connect mysql: %v", err)
	}
	defer func() { _ = db.Close() }()

	// check if we need to flush at all
	lastFlush := getLastFlushTime(ctx, db)
	if lastFlush.Add(flushInterval).After(time.Now()) {
		log.Printf("binlogs were flushed recently: %v, no need to flush", lastFlush)
		return
	}

	slaveStatus, err := mysqlutil.GetSlaveStatus(ctx, db)
	if err != nil {
		log.Fatalf("failed to get current host slave status: %v", err)
	}
	if slaveStatus == nil {
		// flushing binlogs locks until all pending transactions are committed
		// it may lead to deadlocks while semisync is enabled and master lost
		// the most safe way to commit smthing before flushing binlogs
		log.Printf("running on master, checking ability to commit")
		if !ableToCommit(ctx, db) {
			log.Fatalf("gtid_executed is not changing, flushing is not safe")
		}
	} else {
		// running on slave, safe to flush
		log.Printf("running on slave, flushLogs should be safe")
	}

	flushBinlogs(ctx, db)
}
