package agent

import (
	"github.com/prometheus/client_golang/prometheus"
)

const (
	agentNamespace = "mdb_deploy_agent"
)

var metrics *agentMetrics

func init() {
	metrics = initAgetMetrics()
}

type agentMetrics struct {
	commandsTotal prometheus.Counter
	doneCommands  prometheus.Counter
	backlogSize   prometheus.Gauge
}

func initAgetMetrics() *agentMetrics {
	m := &agentMetrics{
		commandsTotal: prometheus.NewCounter(
			prometheus.CounterOpts{
				Name:      "new_commands",
				Namespace: agentNamespace,
				Help:      "Total number of a commands received by agent",
			}),
		doneCommands: prometheus.NewCounter(
			prometheus.CounterOpts{
				Name:      "done_commands",
				Namespace: agentNamespace,
				Help:      "Total number of done commands",
			}),
		backlogSize: prometheus.NewGauge(
			prometheus.GaugeOpts{
				Name:      "backlog_size",
				Namespace: agentNamespace,
				Help:      "Current size of commands backlog",
			}),
	}
	prometheus.MustRegister(
		m.commandsTotal,
		m.doneCommands,
		m.backlogSize,
	)
	return m
}
