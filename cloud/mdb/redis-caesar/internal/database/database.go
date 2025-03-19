package database

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal"
)

//go:generate $GOPATH/bin/mockgen -source ./database.go -destination ./mock/database.go -package mock

type DB interface {
	// UpdateHostsConnections checks that current connetions to redis are actual to hosts that declared in DCS.
	UpdateHostsConnections(ctx context.Context, info internal.AllDBsInfo) error
	// InitHostsConnections initializes a connection to all hosts that declared in config.
	InitHostsConnections(ctx context.Context)
	// Ping pings all connections.
	Ping(ctx context.Context) error
	PingHost(ctx context.Context, host internal.RedisHost) error
	DatabasesInfo(ctx context.Context) (internal.AllDBsInfo, error)
	PromoteMaster(ctx context.Context, master internal.RedisHost) error
}
