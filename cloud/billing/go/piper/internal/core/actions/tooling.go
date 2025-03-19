package actions

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func ReportLag(
	ctx context.Context, scope entities.ProcessingScope, from time.Time, reportWrite bool,
	metrics []entities.EnrichedMetric,
) error {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, nil)
	}()

	useObs := map[string]tooling.IntObserver{}
	writeObs := map[string]tooling.IntObserver{}
	for i := range metrics {
		schema := metrics[i].Schema
		{
			use, ok := useObs[schema]
			if !ok {
				use = tooling.MetricSchemaUsageLagObserver(ctx, schema)
				useObs[schema] = use
			}
			useLag := from.Sub(metrics[i].Usage.Finish).Seconds()
			use.ObserveInt(int(useLag))
		}

		if reportWrite {
			wr, ok := writeObs[schema]
			if !ok {
				wr = tooling.MetricSchemaWriteLagObserver(ctx, schema)
				writeObs[schema] = wr
			}

			writeLag := from.Sub(metrics[i].MessageWriteTime).Seconds()
			wr.ObserveInt(int(writeLag))
		}
	}
	return nil
}
