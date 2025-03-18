package metrics

import (
	metrics "github.com/ydb-platform/ydb-go-sdk-metrics"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	coreMetrics "a.yandex-team.ru/library/go/core/metrics"
)

// Table makes table.ClientTrace with core metrics publishing
func Table(registry coreMetrics.Registry, opts ...option) trace.Table {
	c := &config{
		registry:  registry,
		namespace: defaultNamespace,
		separator: defaultSeparator,
	}
	for _, o := range opts {
		o(c)
	}

	if c.details == 0 {
		c.details = trace.DetailsAll
	}
	return metrics.Table(c)
}
