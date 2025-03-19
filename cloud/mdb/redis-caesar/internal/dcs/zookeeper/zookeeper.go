package zookeeper

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"regexp"
	"strings"
	"sync"
	"time"

	"github.com/go-zookeeper/zk"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/dcs"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
)

var deduplicateSepRegex = regexp.MustCompile(dcs.Separator + "+")

const (
	retryCount              = 10
	pathManager             = "manager"
	pathPrefixRedisNodes    = "redis_nodes"
	pathDBMaster            = "db_master"
	pathFailoverWaitStarted = "failover_wait_started"
	pathFailoverStarted     = "failover_started"
)

type zkDCS struct {
	logger         log.Logger
	conf           *config.ZKConfig
	conn           *zk.Conn
	eventsChan     <-chan zk.Event
	isConnected    bool
	connectedLock  sync.Mutex
	connectedChans []chan struct{}
}

func New(config *config.ZKConfig, logger log.Logger) (dcs.DCS, error) {
	if len(config.Hosts) == 0 {
		return nil, fmt.Errorf("zookeeper not configured, fill zookeeper/hosts in config")
	}
	if config.Namespace == "" {
		return nil, fmt.Errorf("zookeeper not configured, fill zookeeper/namespace in config")
	}
	if !strings.HasPrefix(config.Namespace, dcs.Separator) {
		return nil, fmt.Errorf("zookeeper namespace should start with /")
	}
	if config.SessionTimeout == 0 {
		return nil, fmt.Errorf("namespace session timeout not configured")
	}
	conn, ec, err := zk.Connect(config.Hosts, config.SessionTimeout)
	if err != nil {
		return nil, fmt.Errorf("unable to connect to ZooKeeper: %w", err)
	}
	z := &zkDCS{
		conf:       config,
		logger:     logger,
		conn:       conn,
		eventsChan: ec,
	}

	return z, nil
}

func (z *zkDCS) IsConnected() bool {
	z.connectedLock.Lock()
	defer z.connectedLock.Unlock()

	return z.isConnected
}

func (z *zkDCS) AcquireManagerLock() bool {
	fullPath := z.buildFullPath(pathManager)
	self := z.getSelfLockOwner()
	data, _, err := z.get(fullPath)
	if err != nil && !errors.Is(err, zk.ErrNoNode) {
		z.logger.Errorf("failed to get lock info %s: %s", fullPath, err.Error())

		return false
	}
	if errors.Is(err, zk.ErrNoNode) {
		data, err = json.Marshal(&self)
		if err != nil {
			panic(fmt.Sprintf("failed to serialize to JSON %#v", self))
		}
		_, err = z.create(fullPath, data, zk.FlagEphemeral, nil)
		if err != nil {
			if !errors.Is(err, zk.ErrNodeExists) {
				z.logger.Errorf("failed to acquire lock %s: %s", fullPath, err.Error())
			}

			return false
		}

		return true
	}
	owner := dcs.LockOwner{}
	if err = json.Unmarshal(data, &owner); err != nil {
		z.logger.Errorf("malformed lock data %s (%s): %s", fullPath, data, err.Error())

		return false
	}

	return owner == self
}

func (z *zkDCS) WaitConnected(timeout time.Duration) bool {
	z.connectedLock.Lock()
	if z.isConnected {
		z.connectedLock.Unlock()

		return true
	}
	c := make(chan struct{})
	z.connectedChans = append(z.connectedChans, c)
	z.connectedLock.Unlock()
	t := time.NewTimer(timeout)
	select {
	case <-c:
		return true
	case <-t.C:
		z.logger.Errorf("failed to connect to DCS within %s", timeout)

		return false
	}
}

func (z *zkDCS) Initialize() {
	err := z.createPath(z.conf.Namespace)
	if err != nil {
		z.logger.Errorf("failed create root path %s : %s", z.conf.Namespace, err.Error())
	}
}

func (z *zkDCS) DatabasesInfo() (internal.AllDBsInfo, error) {
	children, err := z.Children(pathPrefixRedisNodes)
	if err != nil {
		return nil, fmt.Errorf("unable to get databases info paths from zookeeper: %w", err)
	}
	result := internal.AllDBsInfo{}
	for _, child := range children {
		data, _, err := z.get(child)
		if err != nil {
			return nil, fmt.Errorf("unable to get database info from zookeeper: %w", err)
		}
		info := internal.DBInfo{}
		err = json.Unmarshal(data, &info)
		if err != nil {
			return nil, fmt.Errorf("unable to unmarshal database info: %w", err)
		}
		result[info.Host] = info
	}

	return nil, nil
}

func (z *zkDCS) Children(path string) ([]string, error) {
	fullPath := z.buildFullPath(path)
	children, _, err := z.children(fullPath)
	if errors.Is(err, zk.ErrNoNode) {
		return nil, dcs.NewNotFoundError(fullPath)
	}
	if err != nil {
		z.logger.Errorf("failed to get children of %s: %s", fullPath, err.Error())

		return nil, err
	}

	return children, nil
}

func (z *zkDCS) DBMaster() (internal.RedisHost, error) {
	fullPath := z.buildFullPath(pathDBMaster)
	data, _, err := z.get(fullPath)
	if errors.Is(err, zk.ErrNoNode) {
		return "", dcs.NewNotFoundError(fullPath)
	}
	if err != nil {
		z.logger.Errorf("failed to get DB master at path %s: %w", fullPath, err.Error())

		return "", err
	}

	return internal.RedisHost(data), nil
}

func (z *zkDCS) SetDBMaster(master internal.RedisHost) error {
	fullPath := z.buildFullPath(pathDBMaster)
	err := z.createPath(fullPath)
	if err != nil {
		return fmt.Errorf("unable to create DBMaster path: %w", err)
	}

	_, err = z.set(fullPath, []byte(master))
	if err != nil {
		return fmt.Errorf("unable to write master to DCS: %w", err)
	}

	return nil
}

func (z *zkDCS) StartFailoverWait(timestamp int64, master internal.RedisHost) error {
	fullPath := z.buildFullPath(pathFailoverWaitStarted)

	failoverWaiting := dcs.FailoverInfo{
		Timestamp: timestamp,
		OldMaster: master,
	}

	data, err := json.Marshal(&failoverWaiting)
	if err != nil {
		return fmt.Errorf("failed to serialize to JSON: %w", err)
	}

	_, err = z.create(fullPath, data, 0, nil)

	if err != nil {
		return fmt.Errorf("unable to write failover wait data: %w", err)
	}

	return nil
}

func (z *zkDCS) FailoverWaitStarted() (dcs.FailoverInfo, error) {
	fullPath := z.buildFullPath(pathFailoverStarted)
	failoverWaiting := dcs.FailoverInfo{}

	data, _, err := z.get(fullPath)
	if err != nil {
		if errors.Is(err, zk.ErrNoNode) {
			return failoverWaiting, dcs.NewNotFoundError(fullPath)
		}

		return failoverWaiting, fmt.Errorf("unable to get failover wait data: %w", err)
	}

	err = json.Unmarshal(data, &failoverWaiting)
	if err != nil {
		return failoverWaiting, fmt.Errorf("failed to deserialize JSON: %w", err)
	}

	return failoverWaiting, nil
}

func (z *zkDCS) StartFailover(timestamp int64, master internal.RedisHost) error {
	fullPath := z.buildFullPath(pathFailoverStarted)

	failover := dcs.FailoverInfo{
		Timestamp: timestamp,
		OldMaster: master,
	}

	data, err := json.Marshal(&failover)
	if err != nil {
		return fmt.Errorf("failed to serialize to JSON: %w", err)
	}

	_, err = z.create(fullPath, data, 0, nil)

	if err != nil {
		return fmt.Errorf("unable to write failover data: %w", err)
	}

	return nil
}

func (z *zkDCS) FailoverStarted() (dcs.FailoverInfo, error) {
	fullPath := z.buildFullPath(pathFailoverStarted)
	failover := dcs.FailoverInfo{}

	data, _, err := z.get(fullPath)
	if err != nil {
		if errors.Is(err, zk.ErrNoNode) {
			return failover, dcs.NewNotFoundError(fullPath)
		}

		return failover, fmt.Errorf("unable to get failover data %w", err)
	}

	err = json.Unmarshal(data, &failover)
	if err != nil {
		return failover, fmt.Errorf("failed to deserialize JSON: %w", err)
	}

	return failover, nil
}

func (z *zkDCS) set(path string, data []byte) (stat *zk.Stat, err error) {
	z.retry(func() error {
		stat, err = z.conn.Set(path, data, 0)

		return err
	})

	return
}

func (z *zkDCS) createPath(path string) error {
	parts := strings.Split(path, dcs.Separator)
	prefix := ""
	for _, part := range parts {
		if part == "" {
			continue
		}
		prefix = strings.Join([]string{prefix, part}, dcs.Separator)
		returnPath, err := z.create(prefix, []byte{}, 0, nil)
		if err != nil && !errors.Is(err, zk.ErrNodeExists) {
			return fmt.Errorf("unable to create path %s: %w", returnPath, err)
		}
	}

	return nil
}

func (z *zkDCS) retry(callback func() error) {
	for i := 0; i < retryCount+1; i++ {
		err := callback()
		if !errors.Is(err, zk.ErrConnectionClosed) {
			return
		}
		if !z.IsConnected() {
			return
		}
		time.Sleep(z.conf.SessionTimeout / retryCount)
	}
}

func (z *zkDCS) getSelfLockOwner() dcs.LockOwner {
	return dcs.LockOwner{
		Hostname: z.conf.SelfHostname,
		Pid:      os.Getgid(),
	}
}

func (z *zkDCS) get(path string) (data []byte, stat *zk.Stat, err error) {
	z.retry(func() error {
		data, stat, err = z.conn.Get(path)

		return err
	})

	return
}

func (z *zkDCS) create(path string, data []byte, flags int32, acl []zk.ACL) (rpath string, err error) {
	if acl == nil {
		acl = zk.WorldACL(zk.PermAll)
	}
	z.retry(func() error {
		rpath, err = z.conn.Create(path, data, flags, acl)

		return err
	})

	return
}

func (z *zkDCS) buildFullPath(path string) string {
	if path == "" || path == dcs.Separator {
		return z.conf.Namespace
	}
	fullPath := fmt.Sprintf("%s/%s", z.conf.Namespace, path)
	fullPath = strings.TrimSuffix(fullPath, dcs.Separator)
	fullPath = deduplicateSepRegex.ReplaceAllString(fullPath, dcs.Separator)

	return fullPath
}

func (z *zkDCS) children(path string) (children []string, stat *zk.Stat, err error) {
	z.retry(func() error {
		children, stat, err = z.conn.Children(path)

		return err
	})

	return
}
