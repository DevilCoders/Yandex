package main

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"sync"
	"time"

	"github.com/jmoiron/sqlx"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/dbaasutil"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/mysqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/mysyncutil"
)

var verbose bool
var dryRun bool
var timeout time.Duration

// user setting, aka mdb_preserve_binlog_bytes
var preserveConfig int64
var dbaasConfig string
var defaultsFile string
var walgCacheFile string
var mysyncInfoFilePath string

// fixed margin to handle possible calculation mistakes
const safetyMargin = 200 * 1024 * 1024

var criticalDiskUsage = int64(95)

func init() {
	pflag.BoolVarP(&verbose, "verbose", "v", false, "more logging")
	pflag.BoolVarP(&dryRun, "dry-run", "d", false, "dry run")
	pflag.DurationVarP(&timeout, "timeout", "t", 1*time.Minute, "timeout for all operations")
	pflag.Int64VarP(&preserveConfig, "preserve-bytes", "s", 1024*1024*1024, "size of binary logs to preserve, even if unused")
	pflag.Int64VarP(&criticalDiskUsage, "critical-usage", "c", 95, "critical disk usage to enter read-only")
	pflag.StringVar(&dbaasConfig, "dbaas-config", "/etc/dbaas.conf", "dbaas cluster config file")
	pflag.StringVar(&defaultsFile, "defaults-file", "", "mysql client settings file")
	pflag.StringVar(&walgCacheFile, "walg-cache-file", "/home/mysql/.walg_mysql_binlogs_cache", "file with wal-g binlogs backup info")
	pflag.StringVar(&mysyncInfoFilePath, "mysync-info-file", "/var/run/mysync/mysync.info", "mysync.info file path")
}

func getLastArchivedBinlog() string {
	var cache struct{ LastArchivedBinlog string }
	data, err := ioutil.ReadFile(walgCacheFile)
	if err != nil {
		if os.IsNotExist(err) {
			return ""
		}
		log.Fatalf("failed to read wal-g cache file: %v", err)
	}
	err = json.Unmarshal(data, &cache)
	if err != nil {
		log.Fatalf("failed to wal-g parse cache file: %v", err)
	}
	return cache.LastArchivedBinlog
}

// Returns minimum for the binlog list, ignoring empty ones
func minBinlog(binlogs ...string) string {
	min := ""
	for _, binlog := range binlogs {
		if binlog == "" {
			continue
		}
		if min == "" || binlog < min {
			min = binlog
		}
	}
	return min
}

func getLastUnusedBinlog(usedBinlogs []string) string {
	minUsed := minBinlog(usedBinlogs...)
	if minUsed == "" {
		return ""
	}
	return mysqlutil.PrevBinlog(minUsed)
}

// Given list of binlogs and some binlog name
// Returns total size of all binlogs after passed one
func getPreserveBytes(binlogs []mysqlutil.Binlog, lastUnusedBinlog string) int64 {
	var size int64
	for _, binlog := range binlogs {
		if binlog.Name > lastUnusedBinlog {
			size += binlog.Size
		}
	}
	return size
}

// Given server binlog list and number of bytes to preserve return binlog name to purge to
func unstepBinlog(binlogs []mysqlutil.Binlog, preserveBytes int64) string {
	idx := len(binlogs) - 1
	for idx >= 0 && preserveBytes > 0 {
		preserveBytes -= binlogs[idx].Size
		idx--
	}
	if idx < 0 {
		return ""
	}
	return binlogs[idx].Name
}

func totalBinlogSize(binlogs []mysqlutil.Binlog) int64 {
	total := int64(0)
	for _, bl := range binlogs {
		total += bl.Size
	}
	return total
}

func getCriticalDiskUsage() float64 {
	percent := float64(criticalDiskUsage) / float64(100)
	if percent > 1.0 {
		percent = 1.0
	}
	if percent < 0.5 {
		percent = 0.5
	}
	return percent
}

func max(a, b int64) int64 {
	if a > b {
		return a
	}
	return b
}

func calcEffectivePreserveBytes(host string, disk mysyncutil.DiskState, binlogs []mysqlutil.Binlog,
	lastUnusedBinlog string, lostReplicas []string, preserveOnMaster int64) int64 {
	binlogSize := totalBinlogSize(binlogs)
	// 10% before critical disk usage to prevent read-only mode
	criticalPercent := getCriticalDiskUsage() - 0.1
	// total size of binlog to preserve
	preserveBytes := int64(0)
	// how many bytes preserve by user config
	preserveBytes = max(preserveBytes, preserveConfig)
	// how many bytes preserve to keep running replicas and binlog archiving
	preserveUsed := getPreserveBytes(binlogs, lastUnusedBinlog)
	preserveBytes = max(preserveBytes, preserveUsed)
	// how many bytes preserve to support failover from master without replication and archiving breakdown
	// for master host it's the same as preserveUsed
	preserveBytes = max(preserveBytes, preserveOnMaster)
	// how many bytes preserve to keep lost replicas
	preserveConservative := int64(0)
	if len(lostReplicas) > 0 {
		// free space that may be used for keeping binlog
		// it may be zero or negative, meaning that binlog already all allowed space
		freeWritableSpace := int64(criticalPercent*float64(disk.Total) - float64(disk.Used))
		preserveConservative = binlogSize + freeWritableSpace
		preserveBytes = max(preserveBytes, preserveConservative)
		if verbose {
			log.Printf("%s: has lost replicas: %v and free writable space %d", host, lostReplicas, freeWritableSpace)
		}
	}
	if verbose {
		log.Printf("%s: binlogsSize %d preserveConfig %d preserveUsed %d preserveOnMaster %d preserveConservative %d",
			host, binlogSize, preserveConfig, preserveUsed, preserveOnMaster, preserveConservative)
		log.Printf("%s: effective preserveBytes: %d", host, preserveBytes)
	}
	return preserveBytes
}

func purgeBinlogs(_ context.Context, db *sqlx.DB, host, safeToPurgeBinlog string) {
	if safeToPurgeBinlog == "" {
		if verbose {
			log.Printf("%s: there are no binlogs to purge", host)
		}
		return
	}
	purgeTo := mysqlutil.NextBinlog(safeToPurgeBinlog)
	if verbose {
		log.Printf("%s: purging binlogs to %s", host, purgeTo)
	}
	query := fmt.Sprintf("PURGE BINARY LOGS TO '%s'", purgeTo)
	var err error
	if !dryRun {
		_, err = db.Exec(query)
	}
	if err != nil {
		log.Printf("%s: failed to purge binlogs: %v", host, err)
		return
	}
	log.Printf("done")
}

func getSlaveStatusesInParallel(hosts []string, getter func(string) (*mysqlutil.SlaveStatus, error)) map[string]*mysqlutil.SlaveStatus {
	type result struct {
		name   string
		status *mysqlutil.SlaveStatus
		err    error
	}
	results := make(chan result, len(hosts))
	for _, host := range hosts {
		go func(host string) {
			status, err := getter(host)
			results <- result{host, status, err}
		}(host)
	}
	// combine results:
	clusterState := make(map[string]*mysqlutil.SlaveStatus)
	for range hosts {
		result := <-results
		if result.err == nil && result.status != nil {
			clusterState[result.name] = result.status
		}
	}
	return clusterState
}

func calcLostReplicas(statuses map[string]*mysqlutil.SlaveStatus, checkedHosts []string) []string {
	res := make([]string, 0)
	for _, host := range checkedHosts {
		status, ok := statuses[host]
		if !ok || status.SlaveIORunning != "Yes" {
			res = append(res, host)
		}
	}
	return res
}

func main() {
	pflag.Parse()
	parentCtx := context.Background()
	parentCtx = signals.WithCancelOnSignal(parentCtx)
	slaveCtx, scancel := context.WithTimeout(parentCtx, timeout>>1)
	defer scancel()
	ctx, mcancel := context.WithTimeout(parentCtx, timeout)
	defer mcancel()

	if dryRun {
		log.Printf("running in dry-run mode")
	}

	masterDB, err := mysqlutil.ConnectWithDefaultsFile(defaultsFile)
	if err != nil {
		log.Fatalf("failed to connect mysql: %v", err)
	}
	defer func() { _ = masterDB.Close() }()

	// check we are on master
	slaveStatus, err := mysqlutil.GetSlaveStatus(slaveCtx, masterDB)
	if err != nil {
		log.Fatalf("failed to get current host slave status: %v", err)
	}
	if slaveStatus != nil {
		if verbose {
			log.Printf("this script should be run on master")
		}
		return
	}

	masterHost, err := os.Hostname()
	if err != nil {
		log.Fatalf("failed to get hostname: %s", err)
	}

	config, err := dbaasutil.ReadDbaasConfig(dbaasConfig)
	if err != nil {
		log.Fatal(err)
	}
	// collect slave status in parallel
	slaveStatuses := getSlaveStatusesInParallel(config.ClusterHosts, func(host string) (*mysqlutil.SlaveStatus, error) {
		if host == masterHost {
			// we already have masterDB
			return nil, errors.New("not a slave")
		}
		db, err := mysqlutil.ConnectWithDefaultsFileAndHost(defaultsFile, host)
		if err != nil {
			log.Printf("failed to connect mysql: %v", err)
			return nil, err
		}
		defer func(db *sqlx.DB) { _ = db.Close() }(db)
		if err := mysqlutil.Ping(slaveCtx, db); err != nil {
			log.Printf("failed to ping slave %s: %v", host, err)
			log.Printf("skipping... %s may need resetup later", host)
			return nil, err
		}
		slaveStatus, err := mysqlutil.GetSlaveStatus(slaveCtx, db)
		if err != nil {
			log.Fatalf("failed to get slave status for %s", host)
			return nil, err
		}
		if slaveStatus == nil {
			log.Printf("empty slave status for %s\n", host)
			return nil, errors.New("empty slave status")
		}
		return slaveStatus, nil
	})

	slaveUsedBinlogs := make(map[string][]string)
	for _, status := range slaveStatuses {
		if status != nil {
			slaveUsedBinlogs[status.MasterHost] = append(slaveUsedBinlogs[status.MasterHost], status.MasterLogFile)
		}
	}

	mi, err := mysyncutil.ReadMysyncInfoFile(mysyncInfoFilePath)
	if err != nil && verbose {
		log.Fatalf("mysync.info read error %s: %v", mysyncInfoFilePath, err)
	}

	// Estimate how many bytes Master hasn't uploaded to S3 or used by other replicas:
	// we should preserve it on HA replicas too in order to keep replication and archiving after failover
	lastArchivedBinlog := getLastArchivedBinlog()
	lastUnusedBinlog := getLastUnusedBinlog(slaveUsedBinlogs[masterHost])
	lastUnusedAndArchivedBinlog := minBinlog(lastUnusedBinlog, lastArchivedBinlog)
	if verbose {
		log.Printf("master: last archived binlog: %s", lastArchivedBinlog)
		log.Printf("master: last unused binlog: %s", lastUnusedBinlog)
		log.Printf("master: last unused and archived binlog: %s", lastUnusedAndArchivedBinlog)
	}
	masterBinlogs, err := mysqlutil.GetBinlogs(ctx, masterDB)
	if err != nil || len(masterBinlogs) == 0 {
		log.Fatalf("master: failed to get binlog list: %v", err)
	}
	preserveOnMaster := getPreserveBytes(masterBinlogs, lastUnusedAndArchivedBinlog)
	if verbose {
		log.Printf("master: preserve bytes on master: %d", preserveOnMaster)
	}

	// Purge binlogs in parallel:
	var wg sync.WaitGroup
	for _, host := range config.ClusterHosts {
		wg.Add(1)
		go func(host string) {
			defer wg.Done()
			db, err := mysqlutil.ConnectWithDefaultsFileAndHost(defaultsFile, host)
			if err != nil {
				log.Printf("%s: failed to connect mysql: %v", host, err)
				return
			}
			defer func(db *sqlx.DB) { _ = db.Close() }(db)

			binlogs, err := mysqlutil.GetBinlogs(ctx, db)
			if err != nil || len(binlogs) == 0 {
				log.Printf("%s: failed to get binlog list: %v", host, err)
				return
			}

			ds, err := mi.DiskState(host)
			if err != nil {
				log.Printf("%s: failed to get disk state: %v", host, err)
				return
			}

			lastUnusedBinlog := mysqlutil.PrevBinlog(binlogs[len(binlogs)-1].Name) // newest but one binlog
			if host == masterHost {
				// take un-archived logs into consideration
				lastUnusedBinlog = minBinlog(lastUnusedBinlog, lastArchivedBinlog)
			}
			// We have hosts that are replicating from current one
			if len(slaveUsedBinlogs[host]) != 0 {
				lastUnusedBinlog = minBinlog(lastUnusedBinlog, getLastUnusedBinlog(slaveUsedBinlogs[host]))
			}
			if verbose {
				log.Printf("%s: last unsed binlog: %s", host, lastUnusedBinlog)
			}

			// find hosts that should replicate from us, but do not replicat at all
			myExpectedReplicas, err := mi.ExpectedReplicasOf(host)
			if err != nil {
				log.Printf("%s: failed to get expected replicas: %v", host, err)
				return
			}
			lostReplicas := calcLostReplicas(slaveStatuses, myExpectedReplicas)

			// calculate with binlog we should purge to and purge it
			preserveBytes := calcEffectivePreserveBytes(host, ds, binlogs, lastUnusedBinlog, lostReplicas, preserveOnMaster)
			binlogToPurge := unstepBinlog(binlogs, preserveBytes+safetyMargin)
			purgeBinlogs(ctx, db, host, binlogToPurge)
		}(host)
	}

	wg.Wait()
	if verbose {
		log.Printf("Done")
	}
}
