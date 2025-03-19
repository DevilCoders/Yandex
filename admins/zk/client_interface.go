// Package zk provides safe zk client wrapper with autoretry
package zk

import (
	"context"

	"github.com/go-zookeeper/zk"
)

// Client interface to Zk
type Client interface {
	Lock(context.Context, string) (*zk.Lock, error)
	LockWithData(context.Context, string, []byte) (*zk.Lock, error)
	Create(context.Context, string, []byte, int32, []zk.ACL) (string, error)
	Children(context.Context, string) ([]string, *zk.Stat, error)
	ChildrenW(context.Context, string) ([]string, *zk.Stat, <-chan zk.Event, error)
	Get(context.Context, string) ([]byte, *zk.Stat, error)
	GetW(context.Context, string) ([]byte, *zk.Stat, <-chan zk.Event, error)
	Set(context.Context, string, []byte, int32) (*zk.Stat, error)
	Exists(context.Context, string) (bool, *zk.Stat, error)
	ExistsW(context.Context, string) (bool, *zk.Stat, <-chan zk.Event, error)
	Delete(context.Context, string, int32) error
	EnsureZkPath(context.Context, string) error
	EnsureZkPathCached(context.Context, string) error
	Close()
}
