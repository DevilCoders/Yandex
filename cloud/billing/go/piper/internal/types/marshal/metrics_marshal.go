package marshal

import (
	"encoding/json"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var ErrMarshal = errsentinel.New("json marshal")

func MetricJSON(m entities.Metric) (types.JSONAnything, error) {
	if m == nil {
		return types.JSONAnything("null"), nil
	}
	switch mtr := m.(type) {
	case entities.SourceMetric:
		return SourceMetric(mtr)
	case entities.EnrichedMetric:
		return EnrichedMetric(mtr)
	case entities.InvalidMetric:
		return MetricJSON(mtr.Metric)
	default:
		return types.JSONAnything("null"), nil
	}
}

func InvalidMetricJSON(scope entities.ProcessingScope, mtr entities.InvalidMetric) (types.JSONAnything, error) {
	metric := types.QueueError{}
	metric.SequenceID = mtr.MessageOffset + 1
	metric.Reason = mtr.Reason.String()
	metric.SourceID = fmt.Sprintf("%s:%s", scope.SourceType, scope.SourceID)
	metric.Hostname = scope.Hostname
	metric.ReasonComment = mtr.ReasonComment
	metric.SourceName = scope.SourceName
	metric.UploadedAt = types.JSONTimestamp(mtr.UploadedAt())
	metric.RawMetric = string(mtr.RawMetric)

	ids := mtr.MetricID()
	data, err := MetricJSON(mtr)
	if err != nil {
		return types.JSONAnything("null"), ErrMarshal.Wrap(err)
	}

	metric.MetricID = ids.ID
	metric.MetricSchema = ids.Schema
	metric.MetricSourceID = ids.SourceID
	metric.MetricResourceID = ids.ResourceID
	metric.Metric = string(data)

	result, err := json.Marshal(metric)
	if err != nil {
		return types.JSONAnything("null"), ErrMarshal.Wrap(err)
	}
	return types.JSONAnything(result), nil
}

func SourceMetric(m entities.SourceMetric) (types.JSONAnything, error) {
	metric := types.SourceMetric{}
	metric.ID = m.MetricID
	metric.Schema = m.Schema
	metric.CloudID = m.CloudID
	metric.FolderID = m.FolderID
	metric.AbcFolderID = m.AbcFolderID
	metric.BillingAccountID = m.BillingAccountID

	metric.Usage.Quantity = m.Usage.Quantity
	metric.Usage.Type = m.Usage.RawType
	metric.Usage.Unit = m.Usage.Unit
	metric.Usage.Start = types.JSONTimestamp(m.Usage.Start)
	metric.Usage.Finish = types.JSONTimestamp(m.Usage.Finish)

	metric.Tags = m.Tags
	metric.SkuID = m.SkuID
	metric.ResourceID = m.ResourceID
	metric.SourceWriteTime = types.JSONTimestamp(m.SourceWT)
	metric.SourceID = m.SourceID
	metric.Version = m.Version

	metric.UserLabels = make(map[string]types.JSONAnyString)
	if len(m.Labels.User) > 0 {
		for k, v := range m.Labels.User {
			metric.UserLabels[k] = types.JSONAnyString(v)
		}
	}

	result, err := json.Marshal(metric)
	if err != nil {
		return types.JSONAnything("null"), ErrMarshal.Wrap(err)
	}
	return types.JSONAnything(result), nil
}

func EnrichedMetric(m entities.EnrichedMetric) (types.JSONAnything, error) {
	metric := types.ReshardedQueueMetric{}

	metric.ID = m.MetricID
	metric.SequenceID = m.MessageOffset + 1
	metric.Schema = m.Schema
	metric.CloudID = m.CloudID
	metric.FolderID = m.FolderID
	metric.AbcFolderID = m.AbcFolderID
	metric.BillingAccountID = m.BillingAccountID

	metric.Usage.Quantity = m.Usage.Quantity
	metric.Usage.Type = m.Usage.RawType
	metric.Usage.Unit = m.Usage.Unit
	metric.Usage.Start = types.JSONTimestamp(m.Usage.Start)
	metric.Usage.Finish = types.JSONTimestamp(m.Usage.Finish)

	metric.Tags = mergeTags(m.Tags, m.TagsOverride)
	metric.SkuID = m.SkuID
	metric.ResourceID = m.ResourceID
	metric.SourceWriteTime = types.JSONTimestamp(m.SourceWT)
	metric.SourceID = m.SourceID
	metric.Version = m.Version

	metric.ReshardingKey = m.ReshardingKey
	metric.UsageTime = types.JSONTimestamp(m.Usage.UsageTime())
	metric.IsUserLabelsAllowed = true
	metric.MessageWriteTS = types.JSONTimestamp(m.MessageWriteTime)
	metric.PricingQuantity = types.JSONDecimal(m.PricingQuantity)
	metric.PricingUnit = m.PricingUnit
	metric.SkuName = m.SkuName
	metric.StartTime = types.JSONTimestamp(m.Period.Start)
	metric.EndTime = types.JSONTimestamp(m.Period.Finish.Add(-time.Second))

	metric.MasterAccountID = m.MasterAccountID

	metric.UserLabels = make(map[string]string)
	for k, v := range m.Labels.User {
		metric.UserLabels[k] = v
	}

	result, err := json.Marshal(metric)
	if err != nil {
		return types.JSONAnything("null"), ErrMarshal.Wrap(err)
	}
	return types.JSONAnything(result), nil
}

func mergeTags(tags types.JSONAnything, overrides map[string]string) types.JSONAnything {
	if len(overrides) == 0 {
		return tags
	}

	parser := parsers.Get()
	arena := arenas.Get()
	defer parsers.Put(parser)
	defer arenas.Put(arena)

	t, _ := parser.Parse(string(tags))
	for k, v := range overrides {
		val := arena.NewString(v)
		t.Set(k, val)
	}
	return types.JSONAnything(t.MarshalTo(nil))
}
