package mysql

import (
	"bufio"
	"context"
	"crypto/tls"
	"crypto/x509"
	"database/sql"
	"errors"
	"fmt"
	"io/ioutil"
	"math"
	"os"
	"os/exec"
	"regexp"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/go-sql-driver/mysql"
	"github.com/jmoiron/sqlx"
	"github.com/shirou/gopsutil/v3/process"

	"a.yandex-team.ru/cloud/mdb/mysync/internal/config"
	"a.yandex-team.ru/cloud/mdb/mysync/internal/mysql/gtids"
	"a.yandex-team.ru/cloud/mdb/mysync/internal/util"
	"a.yandex-team.ru/library/go/core/log"
)

// Node represents API to query/manipulate single MySQL node
type Node struct {
	config *config.Config
	logger log.Logger
	host   string
	db     *sqlx.DB
}

var queryOnliner = regexp.MustCompile(`\r?\n\s*`)
var mogrifyRegex = regexp.MustCompile(`:\w+`)
var ErrNotLocalNode = errors.New("this method should be run on local node only")

// NewNode returns new Node
func NewNode(config *config.Config, logger log.Logger, host string) (*Node, error) {
	addr := util.JoinHostPort(host, config.MySQL.Port)
	dsn := fmt.Sprintf("%s:%s@tcp(%s)/mysql", config.MySQL.User, config.MySQL.Password, addr)
	if config.MySQL.SslCA != "" {
		dsn += "?tls=custom"
	}
	db, err := sqlx.Open("mysql", dsn)
	if err != nil {
		return nil, err
	}
	// Unsafe option allow us to use queries containing fields missing in structs
	// eg. when we running "SHOW SLAVE STATUS", but need only few columns
	db = db.Unsafe()
	db.SetMaxIdleConns(1)
	db.SetMaxOpenConns(3)
	db.SetConnMaxLifetime(3 * config.TickInterval)
	return &Node{
		config: config,
		logger: logger,
		db:     db,
		host:   host,
	}, nil
}

// RegisterTLSConfig loads and register CA file for TLS encryption
func RegisterTLSConfig(config *config.Config) error {
	if config.MySQL.SslCA != "" {
		var pem []byte
		rootCertPool := x509.NewCertPool()
		pem, err := ioutil.ReadFile(config.MySQL.SslCA)
		if err != nil {
			return err
		}
		if ok := rootCertPool.AppendCertsFromPEM(pem); !ok {
			return fmt.Errorf("failed to parse PEM certificate")
		}
		return mysql.RegisterTLSConfig("custom", &tls.Config{RootCAs: rootCertPool})
	}
	return nil
}

// Host returns Node host name
func (n *Node) Host() string {
	return n.host
}

// IsLocal returns true if MySQL Node running on the same host as calling mysync process
func (n *Node) IsLocal() bool {
	return n.host == n.config.Hostname
}

func (n *Node) String() string {
	return n.host
}

// Close closes underlying SQL connection
func (n *Node) Close() error {
	return n.db.Close()
}

func (n *Node) getCommand(name string) string {
	command, ok := n.config.Commands[name]
	if !ok {
		command, ok = defaultCommands[name]
	}
	if !ok {
		panic(fmt.Sprintf("Failed to find command with name '%s'", name))
	}
	return command
}

func (n *Node) runCommand(name string) (int, error) {
	command := n.getCommand(name)
	if !n.IsLocal() {
		panic(fmt.Sprintf("Remote command execution is not supported (%s on %s)", command, n.host))
	}
	shell := util.GetEnvVariable("SHELL", "sh")
	cmd := exec.Command(shell, "-c", command)
	err := cmd.Run()
	ret := cmd.ProcessState.Sys().(syscall.WaitStatus).ExitStatus()
	n.logger.Debugf("running command '%s', retcode: %d, error %s", command, ret, err)
	return ret, err
}

func (n *Node) getQuery(name string) string {
	query, ok := n.config.Queries[name]
	if !ok {
		query, ok = DefaultQueries[name]
	}
	if !ok {
		panic(fmt.Sprintf("Failed to find query with name '%s'", name))
	}
	return query
}

func (n *Node) traceQuery(query string, arg interface{}, result interface{}, err error) {
	query = queryOnliner.ReplaceAllString(query, " ")
	msg := fmt.Sprintf("node %s running query '%s' with args %#v, result: %#v, error: %s", n.host, query, arg, result, err)
	msg = strings.Replace(msg, n.config.MySQL.Password, "********", -1)
	msg = strings.Replace(msg, n.config.MySQL.ReplicationPassword, "********", -1)
	n.logger.Debug(msg)
}

func (n *Node) queryRow(queryName string, arg interface{}, result interface{}) error {
	return n.queryRowWithTimeout(queryName, arg, result, n.config.DBTimeout)
}

func (n *Node) queryRowWithTimeout(queryName string, arg interface{}, result interface{}, timeout time.Duration) error {
	if arg == nil {
		arg = struct{}{}
	}
	query := n.getQuery(queryName)
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()
	rows, err := n.db.NamedQueryContext(ctx, query, arg)
	if err == nil {
		defer func() { _ = rows.Close() }()
		if rows.Next() {
			err = rows.StructScan(result)
		} else {
			err = rows.Err()
			if err == nil {
				err = sql.ErrNoRows
			}
		}
	}
	n.traceQuery(query, arg, result, err)
	return err
}

// nolint: unparam
func (n *Node) queryRows(queryName string, arg interface{}, scanner func(*sqlx.Rows) error) error {
	if arg == nil {
		arg = struct{}{}
	}
	query := n.getQuery(queryName)
	ctx, cancel := context.WithTimeout(context.Background(), n.config.DBTimeout)
	defer cancel()
	rows, err := n.db.NamedQueryContext(ctx, query, arg)
	n.traceQuery(query, arg, rows, err)
	if err != nil {
		return err
	}
	defer func() { _ = rows.Close() }()
	for rows.Next() {
		err = scanner(rows)
		if err != nil {
			break
		}
	}
	return err
}

// nolint: unparam
func (n *Node) execWithTimeout(queryName string, arg map[string]interface{}, timeout time.Duration) error {
	if arg == nil {
		arg = map[string]interface{}{}
	}
	query := n.getQuery(queryName)
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()
	// avoid connection leak on long lock timeouts
	lockTimeout := int64(math.Floor(0.8 * float64(timeout/time.Second)))
	if _, err := n.db.ExecContext(ctx, n.getQuery(querySetLockTimeout), lockTimeout); err != nil {
		n.traceQuery(query, arg, nil, err)
		return err
	}

	_, err := n.db.NamedExecContext(ctx, query, arg)
	n.traceQuery(query, arg, nil, err)
	return err
}

// nolint: unparam
func (n *Node) exec(queryName string, arg map[string]interface{}) error {
	return n.execWithTimeout(queryName, arg, n.config.DBTimeout)
}

func (n *Node) getRunningQueryIds(excludeUsers []string, timeout time.Duration) ([]int, error) {
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()

	query := DefaultQueries[queryGetProcessIds]

	bquery, args, err := sqlx.In(query, excludeUsers)
	if err != nil {
		n.traceQuery(bquery, args, nil, err)
		return nil, err
	}
	rows, err := n.db.QueryxContext(ctx, bquery, args...)
	if err != nil {
		n.traceQuery(bquery, args, nil, err)
		return nil, err
	}
	defer rows.Close()
	var ret []int

	for rows.Next() {
		var currid int
		err := rows.Scan(&currid)

		if err != nil {
			n.traceQuery(bquery, nil, ret, err)
			return nil, err
		}
		ret = append(ret, currid)
	}

	n.traceQuery(bquery, args, ret, nil)

	return ret, nil
}

type schemaname string

func escape(s string) string {
	s = strings.Replace(s, `\`, `\\`, -1)
	s = strings.Replace(s, `'`, `\'`, -1)
	return s
}

// Poorman's sql templating with value quotation
// Because go built-in placeholders don't work for queries like CHANGE MASTER
func Mogrify(query string, arg map[string]interface{}) string {
	return mogrifyRegex.ReplaceAllStringFunc(query, func(n string) string {
		n = n[1:]
		if v, ok := arg[n]; ok {
			switch vt := v.(type) {
			case schemaname:
				return "`" + string(vt) + "`"
			case string:
				return "'" + escape(vt) + "'"
			case int:
				return strconv.Itoa(vt)
			default:
				return "'" + escape(fmt.Sprintf("%s", vt)) + "'"
			}
		} else {
			return n
		}
	})
}

// not all queries may be parametrized with placeholders
func (n *Node) execMogrify(queryName string, arg map[string]interface{}) error {
	query := n.getQuery(queryName)
	query = Mogrify(query, arg)
	ctx, cancel := context.WithTimeout(context.Background(), n.config.DBTimeout)
	defer cancel()
	_, err := n.db.ExecContext(ctx, query)
	n.traceQuery(query, nil, nil, err)
	return err
}

// IsRunning checks if daemon process is running
func (n *Node) IsRunning() (bool, error) {
	if !n.IsLocal() {
		return false, ErrNotLocalNode
	}
	ret, err := n.runCommand(commandStatus)
	return ret == 0, err
}

func (n *Node) getTestDiskUsage(f string) (used uint64, total uint64, err error) {
	data, err := ioutil.ReadFile(f)
	if err != nil {
		return 0, 0, err
	}
	percent, err := strconv.Atoi(strings.TrimSpace(string((data))))
	if err != nil {
		return 0, 0, err
	}
	someSize := 10 * 1024 * 1024 * 1024
	return uint64(float64(percent) / float64(100) * float64(someSize)), uint64(someSize), nil
}

// GetDiskUsage returns datadir usage statistics
func (n *Node) GetDiskUsage() (used uint64, total uint64, err error) {
	if n.config.TestDiskUsageFile != "" {
		return n.getTestDiskUsage(n.config.TestDiskUsageFile)
	}
	if !n.IsLocal() {
		err = ErrNotLocalNode
		return
	}
	var stat syscall.Statfs_t
	err = syscall.Statfs(n.config.MySQL.DataDir, &stat)
	total = uint64(stat.Bsize) * stat.Blocks
	used = total - uint64(stat.Bsize)*stat.Bavail
	return
}

func (n *Node) GetDaemonStartTime() (time.Time, error) {
	if !n.IsLocal() {
		return time.Time{}, ErrNotLocalNode
	}
	pidB, err := ioutil.ReadFile(n.config.MySQL.PidFile)
	if err != nil {
		if os.IsNotExist(err) {
			err = nil
		}
		return time.Time{}, err
	}
	pid, err := strconv.Atoi(strings.TrimSpace(string(pidB)))
	if err != nil {
		return time.Time{}, err
	}
	ps, err := process.NewProcess(int32(pid))
	if err != nil {
		return time.Time{}, err
	}
	createTS, err := ps.CreateTime()
	if err != nil {
		return time.Time{}, err
	}
	return time.Unix(createTS/1000, (createTS%1000)*1000000), nil
}

func (n *Node) GetCrashRecoveryTime() (time.Time, error) {
	if !n.IsLocal() {
		return time.Time{}, ErrNotLocalNode
	}
	fh, err := os.Open(n.config.MySQL.ErrorLog)
	if err != nil {
		return time.Time{}, err
	}
	defer fh.Close()
	recoveryTimeText := ""
	s := bufio.NewScanner(fh)
	for s.Scan() {
		line := s.Text()
		if strings.HasSuffix(line, " Starting crash recovery...") {
			recoveryTimeText = strings.Split(line, " ")[0]
		}
	}
	if recoveryTimeText == "" {
		return time.Time{}, nil
	}
	return time.Parse("2006-01-02T15:04:05.000000-07:00", recoveryTimeText)
}

// Ping checks node health status by executing simple query
func (n *Node) Ping() (bool, error) {
	result := new(pingResult)
	err := n.queryRow(queryPing, nil, result)
	return result.Ok > 0, err
}

// SlaveStatus returns slave status or nil if node is master
func (n *Node) SlaveStatus() (*SlaveStatus, error) {
	return n.SlaveStatusWithTimeout(n.config.DBTimeout)
}

func (n *Node) SlaveStatusWithTimeout(timeout time.Duration) (*SlaveStatus, error) {
	status := new(SlaveStatus)
	err := n.queryRowWithTimeout(querySlaveStatus, nil, status, timeout)
	if err == sql.ErrNoRows {
		return nil, nil
	}
	return status, err
}

// ReplicationLag returns slave replication lag in seconds
// ReplicationLag may return nil without error if lag is unknown (replication not running)
func (n *Node) ReplicationLag() (*float64, error) {
	lag := new(replicationLag)
	err := n.queryRow(queryReplicationLag, nil, lag)
	if err == sql.ErrNoRows {
		// looks like master
		return new(float64), nil
	}
	if lag.Lag.Valid {
		return &lag.Lag.Float64, nil
	}
	// replication not running, assume lag is huge
	return nil, err
}

// GTIDExecuted returns global transaction id executed
func (n *Node) GTIDExecuted() (*GTIDExecuted, error) {
	status := new(GTIDExecuted)
	err := n.queryRow(queryGTIDExecuted, nil, status)
	if err == sql.ErrNoRows {
		return nil, nil
	}
	return status, err
}

// GTIDExecuted returns global transaction id executed
func (n *Node) GTIDExecutedParsed() (gtids.GTIDSet, error) {
	gtid, err := n.GTIDExecuted()
	if err != nil {
		return nil, err
	}

	return gtids.ParseGtidSet(gtid.ExecutedGtidSet), nil
}

// GetBinlogs returns SHOW BINARY LOGS output
func (n *Node) GetBinlogs() ([]Binlog, error) {
	var binlogs []Binlog
	err := n.queryRows(queryShowBinaryLogs, nil, func(rows *sqlx.Rows) error {
		var binlog Binlog
		err := rows.StructScan(&binlog)
		if err != nil {
			return err
		}
		binlogs = append(binlogs, binlog)
		return nil
	})
	return binlogs, err
}

// IsReadOnly returns (true, true) if MySQL Node in (read-only, super-read-only) mode
func (n *Node) IsReadOnly() (bool, bool, error) {
	var ror readOnlyResult
	err := n.queryRow(queryIsReadOnly, nil, &ror)
	return ror.ReadOnly > 0, ror.SuperReadOnly > 0, err
}

// SetReadOnly sets MySQL Node to be read-only
// Setting server read-only may take a while
// as server waits all running commits (not transactions) to be finished
func (n *Node) SetReadOnly() error {
	return n.execWithTimeout(querySetReadonly, nil, n.config.DBSetRoTimeout)
}

// SetReadOnlyWithForce sets MySQL Node to be read-only
// Setting server read-only with force kill all running queries trying to kill problematic ones
// this may take a while as client may start new queries
func (n *Node) SetReadOnlyWithForce(excludeUsers []string) error {

	quit := make(chan bool)
	ticker := time.NewTicker(time.Second)

	go func() {
		for {
			ids, err := n.getRunningQueryIds(excludeUsers, time.Second)
			if err == nil {
				for _, id := range ids {
					_ = n.exec(queryKillQuery, map[string]interface{}{"kill_id": strconv.Itoa(id)})
				}
			}

			select {
			case <-quit:
				return
			case <-ticker.C:
				continue
			}
		}
	}()

	defer func() { quit <- true }()

	return n.execWithTimeout(querySetReadonly, nil, n.config.DBSetRoForceTimeout)
}

// SetReadOnlyWithExtraForce sets MySQL Node to be read-only
// Setting server read-only with force kill all running queries trying to kill problematic ones
// this may take a while as client may start new queries
func (n *Node) SetReadOnlyWithExtraForce(excludeUsers []string) error {
	// IF Waiting for semi-sync ACK from slave
	isBlocked, err := n.isWaitingSemiSyncAck()
	if err != nil {
		n.logger.Errorf("Failed to fetch processlist on %s: %v", n.Host(), err)
		return err
	}
	if isBlocked {
		n.logger.Infof("Mysql %s is waiting for semi-sync ack. Move it offline", n.Host())
		err := n.SetOffline()
		if err != nil {
			n.logger.Errorf("Failed to move node %s offline: %v", n.Host(), err)
			return err
		}
		n.logger.Infof("Mysql %s is waiting for semi-sync ack. Stop semi-sync replication", n.Host())
		err = n.SemiSyncDisable()
		if err != nil {
			n.logger.Errorf("Failed to stop semi-sync replication at node %s: %v", n.Host(), err)
			return err
		}
	}
	err = n.SetReadOnlyWithForce(excludeUsers)
	if err != nil {
		n.logger.Errorf("failed to set node %s read-only: %v", n.Host(), err)
	}
	return err
}

// SetReadOnlyNoSuper sets MySQL Node to read_only=1, but super_read_only=0
// Setting server read-only may take a while
// as server waits all running commits (not transactions) to be finished
func (n *Node) SetReadOnlyNoSuper() error {
	return n.execWithTimeout(querySetReadonlyNoSuper, nil, n.config.DBSetRoTimeout)
}

// SetWritable sets MySQL Node to be writable, eg. disables read-only
func (n *Node) SetWritable() error {
	return n.exec(querySetWritable, nil)
}

// StopSlave stops replication (both IO and SQL threads)
func (n *Node) StopSlave() error {
	return n.execWithTimeout(queryStopSlave, nil, n.config.DBStopSlaveSQLThreadTimeout)
}

// StartSlave starts replication (both IO and SQL threads)
func (n *Node) StartSlave() error {
	return n.exec(queryStartSlave, nil)
}

// StopSlaveIOThread stops IO replication thread
func (n *Node) StopSlaveIOThread() error {
	return n.exec(queryStopSlaveIOThread, nil)
}

// StartSlaveIOThread starts IO replication thread
func (n *Node) StartSlaveIOThread() error {
	return n.exec(queryStartSlaveIOThread, nil)
}

// RestartSlaveIOThread stops IO replication thread
func (n *Node) RestartSlaveIOThread() error {
	err := n.StopSlaveIOThread()
	if err != nil {
		return err
	}
	return n.StartSlaveIOThread()
}

// StopSlaveSQLThread stops SQL replication thread
func (n *Node) StopSlaveSQLThread() error {
	return n.execWithTimeout(queryStopSlaveSQLThread, nil, n.config.DBStopSlaveSQLThreadTimeout)
}

// StartSlaveSQLThread starts SQL replication thread
func (n *Node) StartSlaveSQLThread() error {
	return n.exec(queryStartSlaveSQLThread, nil)
}

// ResetSlaveAll promotes MySQL Node to be master
func (n *Node) ResetSlaveAll() error {
	return n.exec(queryResetSlaveAll, nil)
}

// SemiSyncStatus returns semi sync status
func (n *Node) SemiSyncStatus() (*SemiSyncStatus, error) {
	status := new(SemiSyncStatus)
	err := n.queryRow(querySemiSyncStatus, nil, status)
	if err != nil {
		if err2, ok := err.(*mysql.MySQLError); ok && err2.Number == 1193 {
			// Error: Unknown system variable
			// means semisync plugin is not loaded
			return status, nil
		}
	}
	return status, err
}

// SemiSyncSetMaster set host as semisync master
func (n *Node) SemiSyncSetMaster() error {
	return n.exec(querySemiSyncSetMaster, nil)
}

// SemiSyncSetSlave set host as semisync master
func (n *Node) SemiSyncSetSlave() error {
	return n.exec(querySemiSyncSetSlave, nil)
}

// SemiSyncDisable disables semi_sync_master and semi_sync_slave
func (n *Node) SemiSyncDisable() error {
	return n.exec(querySemiSyncDisable, nil)
}

// SemiSyncSetWaitSlaveCount changes rpl_semi_sync_master_wait_for_slave_count
func (n *Node) SetSemiSyncWaitSlaveCount(c int) error {
	return n.exec(querySetSemiSyncWaitSlaveCount, map[string]interface{}{"wait_slave_count": c})
}

// IsOffline returns current 'offline_mode' variable value
func (n *Node) IsOffline() (bool, error) {
	status := new(offlineModeStatus)
	err := n.queryRow(queryGetOfflineMode, nil, status)
	if err != nil {
		return false, err
	}
	return status.OfflineMode == 1, err
}

// SetOffline turns on 'offline_mode'
func (n *Node) SetOffline() error {
	return n.exec(queryEnableOfflineMode, nil)
}

// SetOnline turns off 'offline_mode'
func (n *Node) SetOnline() error {
	return n.exec(queryDisableOfflineMode, nil)
}

// ChangeMaster changes master of MySQL Node, demoting it to slave
func (n *Node) ChangeMaster(host string) error {
	useSsl := 0
	if n.config.MySQL.ReplicationSslCA != "" {
		useSsl = 1
	}
	return n.execMogrify(queryChangeMaster, map[string]interface{}{
		"host":            host,
		"port":            n.config.MySQL.ReplicationPort,
		"user":            n.config.MySQL.ReplicationUser,
		"password":        n.config.MySQL.ReplicationPassword,
		"ssl":             useSsl,
		"sslCa":           n.config.MySQL.ReplicationSslCA,
		"retryCount":      n.config.MySQL.ReplicationRetryCount,
		"connectRetry":    n.config.MySQL.ReplicationConnectRetry,
		"heartbeatPeriod": n.config.MySQL.ReplicationHeartbeatPeriod,
	})
}

func (n *Node) ReenableEvents() ([]Event, error) {
	var events []Event
	err := n.queryRows(queryListSlavesideDisabledEvents, nil, func(rows *sqlx.Rows) error {
		var event Event
		err := rows.StructScan(&event)
		if err != nil {
			return err
		}
		events = append(events, event)
		return nil
	})
	if err != nil {
		return nil, err
	}
	for _, event := range events {
		err = n.execMogrify(queryEnableEvent, map[string]interface{}{
			"schema": schemaname(event.Schema),
			"name":   schemaname(event.Name),
		})
		if err != nil {
			return nil, err
		}
	}
	return events, nil
}

// isWaitingSemiSyncAck returns true when Master is stuck in 'Waiting for semi-sync ACK from slave' state
func (n *Node) isWaitingSemiSyncAck() (bool, error) {
	type waitingSemiSyncStatus struct {
		IsWaiting bool `db:"IsWaiting"`
	}
	var status waitingSemiSyncStatus
	err := n.queryRow(queryHasWaitingSemiSyncAck, nil, &status)
	return status.IsWaiting, err
}
