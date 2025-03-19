package main

import (
	"context"
	"log"
	"os"
	"time"

	_ "github.com/go-sql-driver/mysql"
	"github.com/jmoiron/sqlx"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/mysqlutil"
)

var verbose bool
var slave bool
var wait time.Duration
var timeout time.Duration
var defaultsFile string
var port int

func init() {
	pflag.BoolVarP(&verbose, "verbose", "v", false, "Some logging")
	pflag.BoolVarP(&slave, "slave", "s", false, "Check if slave is running")
	pflag.DurationVarP(&wait, "wait", "w", 120*time.Second, "Time to wait before giving up, 60s, 1m, etc..")
	pflag.DurationVarP(&timeout, "timeout", "t", 5*time.Second, "Timeout to check database connection")
	pflag.StringVar(&defaultsFile, "defaults-file", "", "mysql client settings file")
	pflag.IntVar(&port, "port", 0, "mysql connection port")
}

func check(ctx context.Context, dsn string) bool {
	db, err := sqlx.Open("mysql", dsn)
	if err != nil {
		log.Fatalf("failed parse dsn: %v", err)
	}
	db = db.Unsafe()

	defer func() { _ = db.Close() }()

	ctx, cancel := context.WithTimeout(ctx, timeout)
	defer cancel()
	err = db.PingContext(ctx)
	if err != nil {
		if verbose {
			log.Printf("failed to connect to database: %v", err)
		}
		return false
	}

	/* check mysql_ping */
	rows, err := db.QueryContext(ctx, "SELECT 1")
	if rows != nil {
		defer rows.Close()
	}
	if err != nil {
		if verbose {
			log.Printf("failed to execute query: %v", err)
		}
		return false
	}

	if !slave {
		return true
	}
	/* check mysql_slave_running */
	status, err := mysqlutil.GetSlaveStatus(ctx, db)
	if err != nil {
		log.Printf("failed to execute query: %v", err)
		return false
	} else if status == nil {
		/* we are on master */
		return true
	}
	ok := status.SlaveIORunning == "Yes" && status.SlaveSQLRunning == "Yes"
	if verbose && status.SlaveIORunning != "Yes" {
		log.Printf("slave IO not running")
	}
	if verbose && status.SlaveSQLRunning != "Yes" {
		log.Printf("slave SQL not running")
	}
	return ok
}

func main() {
	pflag.Parse()
	cnf, err := mysqlutil.ReadDefaultsFile(defaultsFile)
	if err != nil {
		log.Fatalf("failed to read config file: %v", err)
	}
	if port != 0 {
		cnf.Port = port
	}
	dsn := cnf.Dsn()

	ctx := context.Background()
	tctx, cancel := context.WithTimeout(ctx, wait)
	defer cancel()
	ctx = signals.WithCancelOnSignal(tctx)

	if check(ctx, dsn) {
		os.Exit(0)
	}

	ticker := time.NewTicker(timeout)
	for {
		select {
		case <-ctx.Done():
			if tctx.Err() != nil {
				log.Fatalf("failed to connect to database within %v", wait)
				os.Exit(1)
			}
			os.Exit(2)
		case <-ticker.C:
			if check(ctx, dsn) {
				os.Exit(0)
			}
		}
	}
}
