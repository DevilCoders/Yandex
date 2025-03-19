package actions

import (
	"context"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
)

func StartEnrichment(
	ctx context.Context, scope entities.ProcessingScope,
	metrics []entities.SourceMetric,
) (valid []entities.EnrichedMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
	}()

	valid = make([]entities.EnrichedMetric, len(metrics))
	for i, sm := range metrics {
		m := entities.EnrichedMetric{SourceMetric: sm}
		m.Products = extractProducts(m.Tags)
		m.Period = usageHours(sm.Usage)
		m.ResourceBindingType = checkResourceBinding(m.Schema)
		valid[i] = m
	}
	return
}

func extractProducts(tags types.JSONAnything) []string {
	if tags == "" {
		return nil
	}
	parser := parsers.Get()
	defer parsers.Put(parser)

	val, _ := parser.Parse(string(tags))
	jsp := val.GetArray("product_ids")

	if len(jsp) == 0 {
		return nil
	}
	products := make([]string, 0, len(jsp))
	for _, pv := range jsp {
		if pv != nil && pv.Type() == fastjson.TypeString {
			products = append(products, string(pv.GetStringBytes()))
		}
	}
	return products
}

func checkResourceBinding(schema string) entities.ResourceBindingType {
	switch schema {
	case "b2b.tracker.license.v1", "b2b.tracker.activity.v1":
		return entities.TrackerResourceBinding
	default:
		return entities.NoResourceBinding
	}
}

type enrichedCommon struct{}

func (enrichedCommon) makeIncorrectMetric(
	m entities.EnrichedMetric, reason entities.MetricFailReason, comment string,
) entities.InvalidMetric {
	im := entities.InvalidMetric{}
	im.Metric = m
	im.SourceWT = m.SourceWT
	im.MessageWriteTime = m.MessageWriteTime
	im.MessageOffset = m.MessageOffset
	im.Reason = reason
	im.ReasonComment = comment
	return im
}
