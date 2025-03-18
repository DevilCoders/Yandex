package log

import (
	"time"

	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	"a.yandex-team.ru/library/go/core/log"
)

func Discovery(l log.Logger, details trace.Details) (t trace.Discovery) {
	if details&trace.DiscoveryEvents != 0 {
		l = l.WithName("discovery")
		t.OnDiscover = func(info trace.DiscoveryDiscoverStartInfo) func(trace.DiscoveryDiscoverDoneInfo) {
			l.Debug("try to discover",
				log.String("version", version),
			)
			start := time.Now()
			return func(info trace.DiscoveryDiscoverDoneInfo) {
				if info.Error == nil {
					endpoints := make([]string, 0, len(info.Endpoints))
					for _, e := range info.Endpoints {
						endpoints = append(endpoints, e.String())
					}
					l.Debug("discover finished",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Strings("endpoints", endpoints),
					)
				} else {
					l.Error("discover failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Error(info.Error),
					)
				}
			}
		}
	}
	return t
}
