package unifiedagent

import (
	"context"
	"encoding/json"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	unifiedagent "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/unified_agent"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func convert(metric entities.E2EMetric, sensor string) unifiedagent.SolomonMetric {
	return unifiedagent.SolomonMetric{
		Labels: map[string]string{
			"sensor":   sensor,
			"sku_name": metric.SkuName,
			"label":    metric.Label,
		},
		Value:     json.Number(metric.Value.String()),
		Timestamp: metric.UsageTime.Unix(),
	}
}

func (s *Session) PushE2EUsageQuantityMetric(_ context.Context, _ entities.ProcessingScope, metric entities.E2EMetric) error {
	s.quantityMetrics = append(s.quantityMetrics, convert(metric, "sku_usage_qty"))
	return nil
}

func (s *Session) PushE2EPricingQuantityMetric(_ context.Context, _ entities.ProcessingScope, metric entities.E2EMetric) error {
	s.quantityMetrics = append(s.quantityMetrics, convert(metric, "sku_pricing_qty"))
	return nil
}

func (s *Session) FlushE2EQuantityMetrics(ctx context.Context) (err error) {
	logger := tooling.Logger(ctx)
	if len(s.quantityMetrics) == 0 {
		logger.Debug("No solomon messages to send")
		return nil
	}

	err = s.adapter.uaClient.PushMetrics(ctx, s.quantityMetrics...)
	if err != nil {
		return
	}

	s.quantityMetrics = nil
	return nil
}
