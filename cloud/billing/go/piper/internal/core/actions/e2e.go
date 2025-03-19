package actions

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

type E2EQuantityPusher interface {
	PushE2EUsageQuantityMetric(context.Context, entities.ProcessingScope, entities.E2EMetric) error
	PushE2EPricingQuantityMetric(context.Context, entities.ProcessingScope, entities.E2EMetric) error
	FlushE2EQuantityMetrics(ctx context.Context) error
}

const e2eLabelName = "yc-billing-e2e"

func ReportE2EQuantity(
	ctx context.Context, scope entities.ProcessingScope, pusher E2EQuantityPusher,
	metrics []entities.EnrichedMetric,
) (err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() { tooling.ActionDone(ctx, err) }()

	for _, metric := range metrics {
		label := metric.Labels.User[e2eLabelName]
		if label == "" {
			continue
		}

		err = pusher.PushE2EUsageQuantityMetric(ctx, scope, entities.E2EMetric{
			SkuInfo:   metric.SkuInfo,
			Label:     label,
			Value:     metric.Usage.Quantity,
			UsageTime: metric.Usage.UsageTime(),
		})
		if err != nil {
			return err
		}

		err = pusher.PushE2EPricingQuantityMetric(ctx, scope, entities.E2EMetric{
			SkuInfo:   metric.SkuInfo,
			Label:     label,
			Value:     metric.PricingQuantity,
			UsageTime: metric.Usage.UsageTime(),
		})
		if err != nil {
			return err
		}
	}

	return pusher.FlushE2EQuantityMetrics(ctx)
}
