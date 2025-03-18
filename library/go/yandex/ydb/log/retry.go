package log

import (
	"time"

	"github.com/ydb-platform/ydb-go-sdk/v3"
	ydbRetry "github.com/ydb-platform/ydb-go-sdk/v3/retry"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	"a.yandex-team.ru/library/go/core/log"
)

func Retry(l log.Logger, details trace.Details) (t trace.Retry) {
	if details&trace.RetryEvents != 0 {
		retry := l.WithName("retry")
		t.OnRetry = func(info trace.RetryLoopStartInfo) func(trace.RetryLoopIntermediateInfo) func(trace.RetryLoopDoneInfo) {
			idempotent := info.Idempotent
			retry.Debug("init",
				log.String("version", version),
				log.Bool("idempotent", idempotent))
			start := time.Now()
			return func(info trace.RetryLoopIntermediateInfo) func(doneInfo trace.RetryLoopDoneInfo) {
				if info.Error == nil {
					retry.Debug("attempt",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Bool("idempotent", idempotent),
					)
				} else {
					f := retry.Warn
					if !ydb.IsYdbError(info.Error) {
						f = retry.Debug
					}
					m := ydbRetry.Check(info.Error)
					f("intermediate",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Bool("idempotent", idempotent),
						log.Bool("retryable", m.MustRetry(idempotent)),
						log.Bool("deleteSession", m.MustDeleteSession()),
						log.Int64("code", m.StatusCode()),
						log.Error(info.Error),
					)
				}
				return func(info trace.RetryLoopDoneInfo) {
					if info.Error == nil {
						retry.Debug("finish",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Bool("idempotent", idempotent),
							log.Int("attempts", info.Attempts),
						)
					} else {
						f := retry.Error
						if !ydb.IsYdbError(info.Error) {
							f = retry.Debug
						}
						m := ydbRetry.Check(info.Error)
						f("done",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Bool("idempotent", idempotent),
							log.Bool("retryable", m.MustRetry(idempotent)),
							log.Bool("deleteSession", m.MustDeleteSession()),
							log.Int64("code", m.StatusCode()),
							log.Error(info.Error),
						)
					}
				}
			}

		}
	}
	return t
}
