package metrics

import (
	metrics "github.com/ydb-platform/ydb-go-sdk-metrics"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	coreMetrics "a.yandex-team.ru/library/go/core/metrics"
)

// Scripting makes trace.Scripting with prometheus metrics publishing
func Scripting(registry coreMetrics.Registry, opts ...option) trace.Scripting {
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
	return metrics.Scripting(c)
}
