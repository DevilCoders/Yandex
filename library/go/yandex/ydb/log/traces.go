package log

import (
	"github.com/ydb-platform/ydb-go-sdk/v3"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	"a.yandex-team.ru/library/go/core/log"
)

func WithTraces(l log.Logger, details trace.Details) ydb.Option {
	return ydb.MergeOptions(
		ydb.WithTraceDriver(Driver(l, details)),
		ydb.WithTraceTable(Table(l, details)),
		ydb.WithTraceScripting(Scripting(l, details)),
		ydb.WithTraceScheme(Scheme(l, details)),
		ydb.WithTraceCoordination(Coordination(l, details)),
		ydb.WithTraceRatelimiter(Ratelimiter(l, details)),
		ydb.WithTraceDiscovery(Discovery(l, details)),
	)
}
