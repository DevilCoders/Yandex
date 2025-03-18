package uazap

import (
	"context"

	"a.yandex-team.ru/library/go/core/metrics"
)

func CoreMetricsCollector(r metrics.Registry, c metrics.CollectPolicy) func(c *Core) {
	return func(core *Core) {
		if r == nil {
			return
		}

		r = r.WithPrefix(r.ComposeName("ua", "log"))

		core.bg.client.CollectMetrics(r, c)

		var stat *Stat
		c.AddCollect(func(context.Context) {
			stat = core.Stat()
		})

		r.FuncCounter(r.ComposeName("messages", "received"), c.RegisteredCounter(func() int64 {
			return stat.ReceivedMessages
		}))
		r.FuncCounter(r.ComposeName("bytes", "received"), c.RegisteredCounter(func() int64 {
			return stat.ReceivedBytes
		}))
		r.FuncCounter(r.ComposeName("messages", "dropped"), c.RegisteredCounter(func() int64 {
			return stat.DroppedMessages
		}))
		r.FuncCounter(r.ComposeName("bytes", "dropped"), c.RegisteredCounter(func() int64 {
			return stat.DroppedBytes
		}))
		r.FuncCounter(r.ComposeName("messages", "inflight"), c.RegisteredCounter(func() int64 {
			return stat.InflightMessages
		}))
		r.FuncCounter(r.ComposeName("bytes", "inflight"), c.RegisteredCounter(func() int64 {
			return stat.InflightBytes
		}))
		r.FuncCounter(r.ComposeName("errors", "count"), c.RegisteredCounter(func() int64 {
			return stat.ErrorsCount
		}))
	}
}
