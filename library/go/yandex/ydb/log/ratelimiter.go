package log

import (
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	"a.yandex-team.ru/library/go/core/log"
)

func Ratelimiter(l log.Logger, details trace.Details) (t trace.Ratelimiter) {
	return t
}
