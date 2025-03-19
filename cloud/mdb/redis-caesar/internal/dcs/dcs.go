package dcs

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal"
)

//go:generate $GOPATH/bin/mockgen -source ./dcs.go -destination ./mock/dcs.go -package mock

// DCS is an interface that describes a set of methods that should implement
// distributed configuration system.
type DCS interface {
	IsConnected() bool
	AcquireManagerLock() bool
	WaitConnected(timeout time.Duration) bool
	Initialize()
	// GetDatabasesInfo retrieves actual info about databases from DCS
	DatabasesInfo() (internal.AllDBsInfo, error)
	Children(path string) ([]string, error)
	DBMaster() (internal.RedisHost, error)
	SetDBMaster(master internal.RedisHost) error
	StartFailoverWait(timestamp int64, master internal.RedisHost) error
	FailoverWaitStarted() (FailoverInfo, error)
	StartFailover(timestamp int64, master internal.RedisHost) error
	FailoverStarted() (FailoverInfo, error)
}

const Separator = "/"

// LockOwner contains info about the process holding the lock.
type LockOwner struct {
	Hostname string `json:"hostname"`
	Pid      int    `json:"pid"`
}

type FailoverInfo struct {
	Timestamp int64              `json:"timestamp"`
	OldMaster internal.RedisHost `json:"old_master"`
}
