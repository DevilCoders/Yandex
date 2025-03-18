package hasqlcollect

import (
	"context"
	"database/sql"

	"golang.yandex/hasql"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/collect"
)

func DBMetrics(dbName string, cluster *hasql.Cluster) collect.Func {
	return func(ctx context.Context, r metrics.Registry, c metrics.CollectPolicy) {
		if r == nil {
			return
		}
		r = r.WithPrefix(r.ComposeName("db", "sql"))

		statsByHost := make(map[string]*sql.DBStats)
		// Nodes are always the same.
		nodes := cluster.Nodes()

		connRegistry := r.WithPrefix("connections").WithTags(map[string]string{"dbname": dbName})
		collectFunc := func(ctx context.Context) {
			for _, node := range nodes {
				stats := statsByHost[node.Addr()]
				if stats == nil {
					stats = &sql.DBStats{}
					statsByHost[node.Addr()] = stats
				}
				*stats = node.DB().Stats()
			}
		}

		collectFunc(ctx)

		// Need to register metrics, because they trigger next collect.
		for host, stats := range statsByHost {
			registerSQLDBMetrics(stats, connRegistry.WithTags(map[string]string{"dbhost": host}), c)
		}

		c.AddCollect(collectFunc)
	}
}

func registerSQLDBMetrics(stats *sql.DBStats, r metrics.Registry, c metrics.CollectPolicy) {
	r.FuncGauge("open", c.RegisteredGauge(func() float64 {
		return float64(stats.OpenConnections)
	}))
	r.FuncGauge(r.ComposeName("open", "max"), c.RegisteredGauge(func() float64 {
		return float64(stats.MaxOpenConnections)
	}))
	r.FuncGauge("inuse", c.RegisteredGauge(func() float64 {
		return float64(stats.InUse)
	}))
	r.FuncGauge("idle", c.RegisteredGauge(func() float64 {
		return float64(stats.Idle)
	}))
	r.FuncCounter(r.ComposeName("wait", "count"), c.RegisteredCounter(func() int64 {
		return stats.WaitCount
	}))
	r.FuncCounter(r.ComposeName("wait", "duration", "ns"), c.RegisteredCounter(func() int64 {
		return int64(stats.WaitDuration)
	}))
	r.FuncCounter(r.ComposeName("closed", "limit", "idle"), c.RegisteredCounter(func() int64 {
		return stats.MaxIdleClosed
	}))
	r.FuncCounter(r.ComposeName("closed", "timeout", "lifetime"),
		c.RegisteredCounter(func() int64 {
			return stats.MaxLifetimeClosed
		}))
	r.FuncCounter(r.ComposeName("closed", "timeout", "idle"), c.RegisteredCounter(func() int64 {
		return stats.MaxIdleTimeClosed
	}))
}
