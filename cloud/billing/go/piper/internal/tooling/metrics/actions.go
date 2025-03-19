package metrics

import (
	"github.com/prometheus/client_golang/prometheus"
)

func init() {
	register(ActionsStarted)
	register(ActionsDone)
}

var (
	ActionsStarted = prometheus.NewGaugeVec(
		prometheus.GaugeOpts{Name: "actions_started"}, []string{"svc", "source", "action"},
	)
	ActionsDone = prometheus.NewGaugeVec(
		prometheus.GaugeOpts{Name: "actions_done"}, []string{"svc", "source", "action", "success"},
	)
)

func ActionStartLabels(service, source, action string) []string {
	return []string{service, source, action}
}

func ActionDoneLabels(service, source, action string, success bool) []string {
	successStr := "no"
	if success {
		successStr = "yes"
	}
	return []string{service, source, action, successStr}
}
