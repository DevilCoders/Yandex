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
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/mysyncutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var verbose bool
var wait time.Duration
var timeout time.Duration
var pollingPeriod time.Duration
var replicaLag time.Duration
var defaultsFile string
var port int
var mysyncInfoFilePath string

func init() {
	pflag.BoolVarP(&verbose, "verbose", "v", false, "Some logging")
	pflag.DurationVarP(&wait, "wait", "w", 5*time.Minute, "Time to wait before giving up, 60s, 1m, etc..")
	pflag.DurationVarP(&timeout, "timeout", "t", 5*time.Second, "Database query timeout")
	pflag.DurationVarP(&pollingPeriod, "polling-period", "p", 5*time.Second, "Polling period")
	pflag.DurationVarP(&replicaLag, "replica-lag", "l", 10*time.Minute, "Replication lag we waiting for")
	pflag.StringVar(&defaultsFile, "defaults-file", "", "mysql client settings file")
	pflag.IntVar(&port, "port", 0, "mysql connection port")
	pflag.StringVar(&mysyncInfoFilePath, "mysync-info-file", "/var/run/mysync/mysync.info", "mysync.info file path")
}

func check(ctx context.Context, dsn string) bool {
	if checkReplicaLag(ctx, dsn) {
		return true
	}
	isMasterReadOnly, err := checkMasterIsReadOnly(mysyncInfoFilePath)
	if err != nil {
		if verbose {
			log.Printf("mysync info read error %s: %v", mysyncInfoFilePath, err)
		}
		return true // master status is unknown, it means that we don't know if we 'synced' or not
	}
	return isMasterReadOnly
}

func checkReplicaLag(ctx context.Context, dsn string) bool {
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

	/* check replica lag */
	lag, err := mysqlutil.GetMdbReplicationLag(ctx, db)
	if err != nil {
		log.Printf("failed to execute query: %v", err)
		return false
	}

	if lag > replicaLag.Seconds() {
		log.Printf("Actual replication lag %v (s) is greater than expected replication lag %v (s)",
			lag, replicaLag.Seconds())
		return false
	}
	return true
}
func checkMasterIsReadOnly(mysyncInfoFilePath string) (bool, error) {
	// read /var/run/mysync/mysync.info
	mi, err := mysyncutil.ReadMysyncInfoFile(mysyncInfoFilePath)
	if err != nil {
		return false, err
	}
	master := mi.GetMaster()
	if master == "" {
		return false, xerrors.New("no master found in mysync.info")
	}
	isReadOnly, err := mi.IsReadOnly(master)
	if err != nil {
		return false, err
	}
	return isReadOnly, nil
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
				log.Fatalf("Replica lag didn't catchup within %v", wait)
				os.Exit(1)
			}
			os.Exit(2)
		case <-ticker.C:
			if check(ctx, dsn) {
				os.Exit(0)
			}
			// TODO: MDB-13930: check if replication is permanently broken and fail fast
		}
	}
}
