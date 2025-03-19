// Package zk provides safe zk client wrapper with autoretry
package zk

import (
	"context"
	"fmt"
	"math/rand"
	"strings"
	"sync"
	"time"

	"github.com/go-zookeeper/zk"

	"a.yandex-team.ru/library/go/core/log"
)

var (
	ErrNoNode     = zk.ErrNoNode
	ErrNodeExists = zk.ErrNodeExists
	ErrNotLocked  = zk.ErrNotLocked
)

// Config for zk client
type Config struct {
	Servers          []string      `yaml:"servers"`
	Timeout          time.Duration `yaml:"timeout"`
	MaxRetryAttempts int           `yaml:"max_retry_attempts"`
}

// Zk safe client
type Zk struct {
	mu              sync.Mutex
	cfg             *Config
	conn            *zk.Conn
	log             log.Logger
	logLevel        log.Level
	stopCh          chan struct{}
	ensurePathCache map[string]struct{}
}

// Printf Implement zk.Logger
func (z *Zk) Printf(fmt string, args ...interface{}) {
	if z.logLevel == log.TraceLevel {
		z.log.Tracef(fmt, args...)
	}
}

// New non thread safe, create new Zook instance
func New(cfg *Config, l log.Logger, logLevel log.Level) *Zk {
	zook := &Zk{
		cfg:             cfg,
		log:             l,
		logLevel:        logLevel,
		stopCh:          make(chan struct{}),
		ensurePathCache: make(map[string]struct{}),
	}
	zook.log.Infof("New zk instance for %v", cfg.Servers)
	zk.DefaultLogger = zook
	return zook
}

// Lock it is blocked until the lock is captured or an error occurs
func (z *Zk) Lock(ctx context.Context, path string) (lock *zk.Lock, err error) {
	return z.LockWithData(ctx, path, []byte{})
}

// LockWithData it is blocked until the lock is captured or an error occurs
func (z *Zk) LockWithData(ctx context.Context, path string, data []byte) (lock *zk.Lock, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		lock = zk.NewLock(conn, path, zk.WorldACL(zk.PermAll))
		err = lock.LockWithData(data)
		return err
	})
	return
}

// Create is part of the zk.Conn interface.
func (z *Zk) Create(ctx context.Context, path string, value []byte, flags int32, acl []zk.ACL) (pathCreated string, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		pathCreated, err = conn.Create(path, value, flags, acl)
		return err
	})
	return
}

// Children is part of the zk.Conn interface.
func (z *Zk) Children(ctx context.Context, path string) (children []string, stat *zk.Stat, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		children, stat, err = conn.Children(path)
		return err
	})
	return
}

// ChildrenW is part of the zk.Conn interface.
func (z *Zk) ChildrenW(ctx context.Context, path string) (children []string, stat *zk.Stat, events <-chan zk.Event, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		children, stat, events, err = conn.ChildrenW(path)
		return err
	})
	return
}

// Get is part of the zk.Conn interface.
func (z *Zk) Get(ctx context.Context, path string) (data []byte, stat *zk.Stat, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		data, stat, err = conn.Get(path)
		return err
	})
	return
}

// GetW is part of the zk.Conn interface.
func (z *Zk) GetW(ctx context.Context, path string) (data []byte, stat *zk.Stat, events <-chan zk.Event, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		data, stat, events, err = conn.GetW(path)
		return err
	})
	return
}

// Set is part of the zk.Conn interface.
func (z *Zk) Set(ctx context.Context, path string, value []byte, version int32) (stat *zk.Stat, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		stat, err = conn.Set(path, value, version)
		return err
	})
	return
}

// Exists is part of the zk.Conn interface.
func (z *Zk) Exists(ctx context.Context, path string) (exists bool, stat *zk.Stat, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		exists, stat, err = conn.Exists(path)
		return err
	})
	return
}

// ExistsW is part of the zk.Conn interface.
func (z *Zk) ExistsW(ctx context.Context, path string) (exists bool, stat *zk.Stat, events <-chan zk.Event, err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		exists, stat, events, err = conn.ExistsW(path)
		return err
	})
	return
}

// Delete is part of the zk.Conn interface.
func (z *Zk) Delete(ctx context.Context, path string, version int32) (err error) {
	err = z.withRetry(ctx, func(conn *zk.Conn) error {
		err = conn.Delete(path, version)
		return err
	})
	return
}

// ensureZkPath is exists
func (z *Zk) ensureZkPath(ctx context.Context, path string, cache bool) error {
	path = strings.Trim(path, "/")
	paths := strings.Split(path, "/")
	rpath := ""
	for _, p := range paths {
		rpath = rpath + "/" + p
		if cache {
			if _, ok := z.ensurePathCache[rpath]; ok {
				continue
			}
			z.ensurePathCache[rpath] = struct{}{}
		}
		_, err := z.Create(ctx, rpath, nil, 0, zk.WorldACL(zk.PermAll))
		if err != nil && err != zk.ErrNodeExists {
			return err
		}
	}
	return nil
}

// EnsureZkPath is exists
func (z *Zk) EnsureZkPath(ctx context.Context, path string) error {
	return z.ensureZkPath(ctx, path, false)
}

// EnsureZkPathCached is EnsureZkPath with global cache
func (z *Zk) EnsureZkPathCached(ctx context.Context, path string) error {
	return z.ensureZkPath(ctx, path, true)
}

// Close zk session
func (z *Zk) Close() {
	select {
	case z.stopCh <- struct{}{}:
		<-z.stopCh
	case <-time.After(1 * time.Millisecond):
	}
}

// withRetry encapsulates the retry logic
func (z *Zk) withRetry(ctx context.Context, action func(conn *zk.Conn) error) (err error) {

	for i := 0; i < z.cfg.MaxRetryAttempts; i++ {
		if i > 0 {
			// Add a bit of backoff time before retrying:
			// 1 second base + up to 5 seconds.
			time.Sleep(1*time.Second + time.Duration(rand.Int63n(5e9)))
			z.log.Debugf("Attempt %d", i)
		}

		// Get the current connection, or connect.
		var conn *zk.Conn
		conn, err = z.getConn(ctx)
		if err != nil {
			// We can't connect, try again.
			z.log.Debugf("Retry getConn, reason: %v", err)
			continue
		}

		// Execute the action.
		err = action(conn)
		if err != zk.ErrConnectionClosed {
			// It worked, or it failed for another reason
			// than connection related.
			return
		}

		// We got an error, because the connection was closed.
		// Let's clear up our errored connection and try again.
		z.mu.Lock()
		if z.conn == conn {
			z.conn = nil
		}
		z.mu.Unlock()
	}
	return
}

// handleSessionEvents is processing events from the session channel.
// When it detects that the connection is not working any more, it
// clears out the connection record.
func (z *Zk) handleSessionEvents(conn *zk.Conn, session <-chan zk.Event) {
	for {
		select {
		case event := <-session:
			switch event.State {
			case zk.StateExpired, zk.StateConnecting, zk.StateDisconnected:
				z.mu.Lock()
				if z.conn == conn {
					// The ZkConn still references this
					// connection, let's nil it.
					z.conn = nil
				}
				z.mu.Unlock()
				z.log.Warnf("zk conn: session ended: %v", event)
				conn.Close()
				return
			case zk.StateHasSession:
				if z.logLevel == log.TraceLevel {
					z.log.Debugf("zk conn: session event: %v", event)
				}
			default:
				z.log.Debugf("zk conn: session event: %v", event)
			}

		case <-z.stopCh:
			z.log.Debugf("zk conn: interrupted by stopCh")
			conn.Close()
			close(z.stopCh)
			return
		}
	}
}

func (z *Zk) getConn(ctx context.Context) (*zk.Conn, error) {
	// zk.Connect automatically shuffles the servers
	z.mu.Lock()
	var conn = z.conn
	z.mu.Unlock()
	if conn != nil {
		return conn, nil
	}
	zconn, session, err := zk.Connect(z.cfg.Servers, z.cfg.Timeout)
	if err != nil {
		return nil, err
	}

	// Wait for connection, skipping transition states.
	for {
		select {
		case <-ctx.Done():
			zconn.Close()
			return nil, ctx.Err()
		case event := <-session:
			switch event.State {
			case zk.StateConnected:
				// success
				z.mu.Lock()
				z.conn = zconn
				z.mu.Unlock()
				go z.handleSessionEvents(zconn, session)
				return zconn, nil
			case zk.StateAuthFailed:
				// fast fail this one
				zconn.Close()
				return nil, fmt.Errorf("zk connect failed: StateAuthFailed")
			case zk.StateConnecting:
				// ignore
			default:
				z.log.Debugf("Got zk session event: %v", event)
			}
		}
	}
}
