package main

import (
	"context"
	"encoding/json"
	"errors"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path"
	"strings"
	"time"

	"github.com/gofrs/flock"
	"github.com/jmoiron/sqlx"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/dbaasutil"
	"a.yandex-team.ru/cloud/mdb/mdb-my-toolkit/pkg/mysqlutil"
)

const yes = "Yes"
const autoCnf = "auto.cnf"
const stopRetries = 3

const highstatePillarDefault = `{"replica": true, "sync-timeout": 43200}`
const highstatePillarForceWalg = `{"replica": true, "sync-timeout": 43200, "walg-restore": true}`
const highstatePillarForceSSH = `{"replica": true, "sync-timeout": 43200, "walg-restore": false}`

var force = false
var forceWalg = false
var forceNoWalg = false
var lockFile = "/var/run/my-resetup.lock"
var stateFile = "/var/run/my-resetup.state"
var resetupFile = "/var/run/mysync/mysync.resetup"
var defaultsFile = ""
var dbaasConfig = "/etc/dbaas.conf"
var mysqlDataDir = "/var/lib/mysql"
var extraCleanupFiles = []string{
	"/tmp/recovery-state",
	"/home/mysql/.restore.my.cnf",
	"/home/mysql/.walg_mysql_binlogs_cache",
}

var highstatePillar = highstatePillarDefault
var notReplicatingTimeout = 24 * time.Hour
var notRunningTimeout = 24 * time.Hour

type MySQLState struct {
	WasRunningAt     time.Time
	WasReplicatingAt time.Time
}

func init() {
	pflag.BoolVar(&force, "force", force, "force timeout checks")
	pflag.BoolVar(&forceWalg, "force-walg", forceWalg, "use walg to resetup")
	pflag.BoolVar(&forceNoWalg, "force-no-walg", forceNoWalg, "don't use walg to resetup")
	pflag.StringVar(&lockFile, "lock-file", lockFile, "resetup lock file")
	pflag.StringVar(&stateFile, "state-file", stateFile, "resetup state file")
	pflag.StringVar(&defaultsFile, "defaults-file", defaultsFile, "mysql client settings file")
	pflag.StringVar(&resetupFile, "resetup-file", resetupFile, "touch file for mysync to force resetup")
	pflag.StringVar(&dbaasConfig, "dbaas-config", "/etc/dbaas.conf", "dbaas cluster config file")
	pflag.StringVar(&mysqlDataDir, "mysql-datadir", mysqlDataDir, "mysql data directory")
	pflag.StringSliceVar(&extraCleanupFiles, "extra-cleanup", extraCleanupFiles, "extra files to clean up before resetup")
	pflag.StringVar(&highstatePillar, "pillar", highstatePillar, "JSON, containing pillar to pass to highstate")
	pflag.DurationVar(&notRunningTimeout, "not-running-timeout", notRunningTimeout, "timeout before dead mysql host will be resetuped")
	pflag.DurationVar(&notReplicatingTimeout, "not-replicating-timeout", notReplicatingTimeout, "timeout before not replicating mysql host will be resetuped")
}

func aquireLock() (*flock.Flock, error) {
	flock := flock.New(lockFile)
	if locked, err := flock.TryLock(); !locked {
		if err != nil {
			return nil, err
		}
		return nil, errors.New("another resetup in progress")
	}
	return flock, nil
}

func readState() *MySQLState {
	state := new(MySQLState)
	stateData, err := ioutil.ReadFile(stateFile)
	if err != nil {
		if os.IsNotExist(err) {
			log.Printf("state file does not exist")
			log.Printf("assuming mysql was OK on previous iteration")
			state.WasRunningAt = time.Now()
			state.WasReplicatingAt = time.Now()
			return state
		}
		log.Fatalf("failed to read state file %s: %s", stateFile, err)
	}
	err = json.Unmarshal(stateData, state)
	if err != nil {
		log.Fatalf("failed to parse state file: %s", err)
	}
	return state
}

func writeState(state *MySQLState) {
	stateData, err := json.Marshal(&state)
	if err != nil {
		log.Fatalf("failed to serialize state: %s", err)
	}
	err = ioutil.WriteFile(stateFile, stateData, 0644)
	if err != nil {
		log.Fatalf("failed to write state file %s: %s", stateFile, err)
	}
}

func stopMysql(ctx context.Context) error {
	var err error
	for i := 0; i < stopRetries; i++ {
		err = exec.CommandContext(ctx, "service", "mysql", "stop").Run()
		if err == nil {
			return nil
		}
	}
	return err
}

func cleanOutDataDir() error {
	dataDir, err := os.Open(mysqlDataDir)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}
	defer func() { _ = dataDir.Close() }()
	files, err := dataDir.Readdirnames(-1)
	if err != nil {
		return err
	}
	for _, file := range files {
		if file == autoCnf {
			// preserve server UUID
			continue
		}
		if err := os.RemoveAll(path.Join(mysqlDataDir, file)); err != nil {
			return err
		}
	}
	return nil
}

func needResetup(ctx context.Context) bool {
	state := readState()
	defer writeState(state)

	db, err := mysqlutil.ConnectWithDefaultsFile(defaultsFile)
	if err != nil {
		log.Fatalf("failed to create mysql connection: %v", err)
	}
	if err = db.PingContext(ctx); err == nil {
		defer func() { _ = db.Close() }()
		state.WasRunningAt = time.Now()
		return needResetupAlive(ctx, db, state)
	}

	log.Printf("mysql is dead")
	return needResetupDead(ctx, state)
}

func resetupFileExists() bool {
	_, err := os.Stat(resetupFile)
	if os.IsNotExist(err) {
		return false
	}
	if err != nil {
		log.Printf("failed to stat %s: %v", resetupFile, err)
		return false
	}
	return true
}

func removeResetupFile() {
	err := os.Remove(resetupFile)
	if err != nil {
		log.Printf("failed to remove %s: %v", resetupFile, err)
		return
	}
	log.Printf("%s removed", resetupFile)
}

func touchResetupFile() {
	f, err := os.OpenFile(resetupFile, os.O_RDONLY|os.O_CREATE, 0644)
	if err != nil {
		log.Printf("failed to touch %s file: %v", resetupFile, err)
		return
	}
	err = f.Close()
	if err != nil {
		log.Printf("failed to close touched file %s: %v", resetupFile, err)
		return
	}
}

func needResetupAlive(ctx context.Context, db *sqlx.DB, state *MySQLState) bool {
	slaveStatus, err := mysqlutil.GetSlaveStatus(ctx, db)
	if err != nil {
		log.Fatalf("failed to get slave status: %v", err)
	}
	if slaveStatus == nil {
		state.WasReplicatingAt = time.Now()
		log.Printf("mysql is running master ok")
		return false
	}

	if resetupFileExists() {
		log.Printf("resetup file exists")
		return true
	}

	if slaveStatus.SlaveIORunning == yes && slaveStatus.SlaveSQLRunning == yes {
		state.WasReplicatingAt = time.Now()
		log.Printf("mysql is running slave replication ok")
		return false
	}

	log.Printf("mysql replication is not ok: SlaveIORunning: %v SlaveSQLRunning: %v, LastIOError: %s LastSQLError: %s",
		slaveStatus.SlaveIORunning, slaveStatus.SlaveSQLRunning, slaveStatus.LastIOError, slaveStatus.LastSQLError)

	// check if replication is lost
	if slaveStatus.SlaveIORunning != yes && isReplicationPermanentlyLost(slaveStatus) {
		return true
	}

	// check if notReplicatingTimeout elapsed
	notReplicatingDuration := time.Since(state.WasReplicatingAt)
	if notReplicatingDuration < notReplicatingTimeout && !force && !resetupFileExists() {
		log.Printf("not replicating timeout is not elapsed yet, remaining %v", notReplicatingTimeout-notReplicatingDuration)
		return false
	}
	log.Printf("mysql replication is not running for %v, resetup now", notReplicatingDuration)
	return true
}

func needResetupDead(ctx context.Context, state *MySQLState) bool {
	notRunningDuration := time.Since(state.WasRunningAt)
	if notRunningDuration < notRunningTimeout && !force && !resetupFileExists() {
		log.Printf("not running timeout is not elapsed yet, remaining %v", notRunningTimeout-notRunningDuration)
		return false
	}
	log.Printf("mysql is dead for %v, ensure it's not master and resetup", notRunningDuration)

	config, err := dbaasutil.ReadDbaasConfig(dbaasConfig)
	if err != nil {
		log.Fatal(err)
	}

	// check that we are not on master
	hostname, err := os.Hostname()
	if err != nil {
		log.Fatalf("failed to get hostname: %v", err)
	}
	for _, host := range config.ClusterHosts {
		if host == hostname {
			continue
		}
		db, err := mysqlutil.ConnectWithDefaultsFileAndHost(defaultsFile, host)
		if err != nil {
			log.Fatalf("failed to create mysql connection: %v", err)
		}
		defer func(db *sqlx.DB) { _ = db.Close() }(db)
		slaveStatus, err := mysqlutil.GetSlaveStatus(ctx, db)
		if err == nil && slaveStatus == nil {
			log.Printf("alive master found at %s, resetup now", host)
			return true
		}
	}
	log.Printf("no alive master found, it's highly possible current host was master, resetup forbidden")
	return false
}

// Last_IO_Errno codes, that disallow to restart replication
var permanentReplicationLostIOErrorCodes = map[int]interface{}{
	// 5.7
	// IO Error 1236 (ER_MASTER_FATAL_ERROR_READING_BINLOG) is a wrapper error
	// check nested errors:
	1236: nil,
	// 8.0
	13114: nil,
}

func isReplicationPermanentlyLost(slaveStatus *mysqlutil.SlaveStatus) bool {
	hasIOErr := hasReplicationError(permanentReplicationLostIOErrorCodes, slaveStatus.LastIOErrno, slaveStatus.LastIOError)
	if hasIOErr {
		log.Printf("mysql replication is permanently lost (Last_IO_Errno = %d), resetup now", slaveStatus.LastIOErrno)
		return true
	}

	return false
}

func hasReplicationError(errorMap map[int]interface{}, errCode int, errStr string) bool {
	if _, ok := errorMap[errCode]; ok {
		return true
	}

	return false
}

func resetupMysql(ctx context.Context) {
	log.Printf("resetup started")
	touchResetupFile()
	log.Printf("stopping mysqld...")
	if err := stopMysql(ctx); err != nil {
		log.Fatalf("failed to stop mysql: %v", err)
	}
	log.Printf("mysqld stopped")
	for _, path := range extraCleanupFiles {
		log.Printf("cleaning %s...", path)
		if err := os.RemoveAll(path); err != nil && !os.IsNotExist(err) {
			log.Fatalf("failed to clean %s: %v", path, err)
		}
		log.Printf("%s cleaned", path)
	}
	log.Printf("cleaning %s...", mysqlDataDir)
	if err := cleanOutDataDir(); err != nil {
		log.Fatalf("failed to clean datadir: %v", err)
	}
	log.Printf("%s cleaned", mysqlDataDir)
	hs := exec.CommandContext(ctx, "salt-call", "state.highstate", "queue=True", "pillar="+highstatePillar, "--retcode-passthrough")
	log.Printf("applying highstate: %s %s", hs.Path, strings.Join(hs.Args, " "))
	if err := hs.Run(); err != nil {
		log.Fatalf("failed to apply highstate: %v", err)
	}
	log.Printf("highstate finished")
	removeResetupFile()
	log.Printf("resetup finished")
}

func main() {
	pflag.Parse()
	if pflag.CommandLine.Changed("force-walg") && pflag.CommandLine.Changed("force-no-walg") {
		log.Fatalf("`--force-walg` and `--force-no-walg` are mutually exclusive. Choose one of them (or non of them)")
	}

	if pflag.CommandLine.Changed("pillar") {
		if !json.Valid([]byte(highstatePillar)) {
			log.Fatalf("highstate-pillar contains invalid JSON: %s", highstatePillar)
		}
	} else if forceWalg {
		highstatePillar = highstatePillarForceWalg
	} else if forceNoWalg {
		highstatePillar = highstatePillarForceSSH
	}

	lock, err := aquireLock()
	if err != nil {
		log.Fatalf("failed to acquire lock: %v", err)
	}
	defer func() { _ = lock.Unlock() }()

	ctx := context.Background()
	ctx = signals.WithCancelOnSignal(ctx)

	if !needResetup(ctx) {
		log.Printf("no need in resetup")
		os.Exit(2)
	}

	resetupMysql(ctx)
}
