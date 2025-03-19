package redis

import (
	"fmt"

	"github.com/go-redis/redis/v8"
	"github.com/prometheus/client_golang/prometheus"
)

type Stats struct {
	master *poolStats
	slave  *poolStats
}

type poolStats struct {
	hits       prometheus.Gauge
	staleConns prometheus.Gauge
	misses     prometheus.Gauge
	idleConns  prometheus.Gauge
	timeouts   prometheus.Gauge
	totalConns prometheus.Gauge
}

var stats = newStats()

func newStats() *Stats {
	s := &Stats{
		master: &poolStats{
			hits:       prometheus.NewGauge(gaugeOpts("master", "hits")),
			staleConns: prometheus.NewGauge(gaugeOpts("master", "staleConns")),
			misses:     prometheus.NewGauge(gaugeOpts("master", "misses")),
			idleConns:  prometheus.NewGauge(gaugeOpts("master", "idleConns")),
			timeouts:   prometheus.NewGauge(gaugeOpts("master", "timeouts")),
			totalConns: prometheus.NewGauge(gaugeOpts("master", "totalConns")),
		},
		slave: &poolStats{
			hits:       prometheus.NewGauge(gaugeOpts("slave", "hits")),
			staleConns: prometheus.NewGauge(gaugeOpts("slave", "staleConns")),
			misses:     prometheus.NewGauge(gaugeOpts("slave", "misses")),
			idleConns:  prometheus.NewGauge(gaugeOpts("slave", "idleConns")),
			timeouts:   prometheus.NewGauge(gaugeOpts("slave", "timeouts")),
			totalConns: prometheus.NewGauge(gaugeOpts("slave", "totalConns")),
		},
	}
	prometheus.MustRegister(
		s.master.hits,
		s.master.staleConns,
		s.master.misses,
		s.master.idleConns,
		s.master.timeouts,
		s.master.totalConns,
		s.slave.hits,
		s.slave.staleConns,
		s.slave.misses,
		s.slave.idleConns,
		s.slave.timeouts,
		s.slave.totalConns,
	)

	return s
}

func gaugeOpts(poolName, metricName string) prometheus.GaugeOpts {
	return prometheus.GaugeOpts{
		Name: fmt.Sprintf("redis_%s_%s", poolName, metricName),
	}
}

func (b *backend) WriteStats() {
	masterStats := b.client.PoolStats()
	slaveStats := b.slaveClient.PoolStats()
	statsToInternal(masterStats, stats.master)
	statsToInternal(slaveStats, stats.slave)
}

func statsToInternal(stats *redis.PoolStats, internalStats *poolStats) {
	internalStats.hits.Set(float64(stats.Hits))
	internalStats.staleConns.Set(float64(stats.StaleConns))
	internalStats.misses.Set(float64(stats.Misses))
	internalStats.idleConns.Set(float64(stats.IdleConns))
	internalStats.timeouts.Set(float64(stats.Timeouts))
	internalStats.totalConns.Set(float64(stats.TotalConns))
}
