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
var wait time.Duration
var timeout time.Duration
var pollingPeriod time.Duration
var defaultsFile string
var port int

func init() {
	pflag.BoolVarP(&verbose, "verbose", "v", false, "Some logging")
	pflag.DurationVarP(&wait, "wait", "w", 5*time.Minute, "Time to wait before giving up, 60s, 1m, etc..")
	pflag.DurationVarP(&timeout, "timeout", "t", 5*time.Second, "Database query timeout")
	pflag.DurationVarP(&pollingPeriod, "polling-period", "p", 5*time.Second, "Polling period")
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

	/* check offline_mode global variable */
	isOffline, err := mysqlutil.IsOffline(ctx, db)
	if err != nil {
		log.Printf("failed to execute query: %v", err)
		return false
	}
	if !isOffline {
		log.Printf("we are online")
		return true
	}

	if verbose {
		log.Printf("we are offline")
	}
	return false
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

	ticker := time.NewTicker(pollingPeriod)
	for {
		select {
		case <-ctx.Done():
			if tctx.Err() != nil {
				log.Fatalf("MySQL havn't changed to offline_mode=OFF within %v", wait)
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
