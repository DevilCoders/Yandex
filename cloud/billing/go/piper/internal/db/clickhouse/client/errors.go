package clickhouseclient

import "a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"

var (
	ErrConnection      = errsentinel.New("connection")
	ErrPing            = errsentinel.New("database access")
	ErrNoSuchPartition = errsentinel.New("no such partition in cluster")
	ErrNoAliveNodes    = errsentinel.New("no alive nodes")
)
