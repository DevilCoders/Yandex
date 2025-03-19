package clickhouse

import (
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var (
	ErrConnection  = errsentinel.New("connection error")
	ErrInvalidPush = errsentinel.New("invalid push error")
)
