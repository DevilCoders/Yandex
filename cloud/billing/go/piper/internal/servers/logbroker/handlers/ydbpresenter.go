package handlers

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

type YDBPresenterTarget interface {
	actions.PresenterPusher
}

type YDBPresenterHandler struct {
	commonHandler
	targetFabric func() YDBPresenterTarget
}

func NewYDBPresenterHandler(
	sourceName string, handlerName string, pipeline string, targetFabric func() YDBPresenterTarget,
) *YDBPresenterHandler {
	return &YDBPresenterHandler{
		commonHandler: newCommonHandler(handlerName, sourceName, pipeline),
		targetFabric:  targetFabric,
	}
}

func (h *YDBPresenterHandler) Handle(ctx context.Context, sourceID lbtypes.SourceID, messages *lbtypes.Messages) {
	ctx, defFunc, scope := h.init(ctx, sourceID, messages)
	defer defFunc()

	if err := h.handle(ctx, scope, messages.Messages); err != nil {
		tooling.TraceTag(ctx, tracetag.Failed())
		tooling.TraceEvent(ctx, "ydb presenter failed", logf.Error(err))
		messages.Error(err)
		return
	}

	messages.Consumed()
}

func (h *YDBPresenterHandler) handle(ctx context.Context, scope entities.ProcessingScope, messages []lbtypes.ReadMessage) error {
	pusher := h.targetFabric()
	items, err := h.parse(ctx, messages)
	if err != nil {
		return err
	}
	tooling.IncomingMetricsCount(ctx, len(items))

	if err := actions.PushPresenterMetrics(ctx, scope, pusher, items); err != nil {
		return err
	}

	tooling.ProcessedMetricsCount(ctx, len(items))
	return nil
}

func (h *YDBPresenterHandler) parse(ctx context.Context, msg []lbtypes.ReadMessage) (items []entities.PresenterMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
	}()

	for _, m := range msg {
		parsed, err := h.parseMessage(ctx, m)
		if err != nil {
			return nil, err
		}
		items = append(items, h.makeEntities(parsed, m.WriteTime, m.Offset)...)
	}
	return
}

func (*YDBPresenterHandler) parseMessage(ctx context.Context, m lbtypes.ReadMessage) (parsed []types.EnrichedQueueMetric, err error) {
	dec := json.NewDecoder(m.DataReader)
	dec.DisallowUnknownFields()
	for dec.More() {
		var sm types.EnrichedQueueMetric
		if err = dec.Decode(&sm); err != nil {
			return nil, fmt.Errorf("source message parse: %w", err)
		}

		// TODO: use sync.Pool with []<ParsedType>
		parsed = append(parsed, sm)
	}
	return
}

func (h *YDBPresenterHandler) makeEntities(items []types.EnrichedQueueMetric, messageWriteTime time.Time, messageOffset uint64) []entities.PresenterMetric {
	if len(items) == 0 {
		return nil
	}

	result := make([]entities.PresenterMetric, 0, len(items))
	for _, item := range items {
		usage := entities.MetricUsage{
			Quantity: item.Usage.Quantity,
			Start:    item.Usage.Start.Time(),
			Finish:   item.Usage.Finish.Time(),
			Unit:     item.Usage.Unit,
			RawType:  item.Usage.Type,
			Type:     jsonUsageTypeToEntity(item.Usage.Type),
		}

		result = append(result, entities.PresenterMetric{
			EnrichedMetric: entities.EnrichedMetric{
				SourceMetric: entities.SourceMetric{
					MetricID:         item.ID,
					Schema:           item.Schema,
					Version:          item.Version,
					CloudID:          item.CloudID,
					FolderID:         item.FolderID,
					AbcID:            item.AbcID,
					AbcFolderID:      item.AbcFolderID,
					ResourceID:       item.ResourceID,
					BillingAccountID: item.BillingAccountID,
					SkuID:            item.SkuID,
					Labels:           jsonSourceLabelsToEntity(item.Labels.User),
					Tags:             item.Tags,
					Usage:            usage,
					SourceID:         item.SourceID,
					SourceWT:         item.SourceWriteTime.Time(),
					MessageWriteTime: messageWriteTime,
					MessageOffset:    messageOffset,
				},
				SkuInfo: entities.SkuInfo{
					SkuName:      item.SkuName,
					PricingUnit:  item.PricingUnit,
					SkuUsageType: usage.Type,
				},
				PricingQuantity: decimal.Decimal128(item.PricingQuantity),
				//Period:              ?,
				//Products:            ?,
				//ResourceBindingType: ?,
				//TagsOverride:        ?,
				MasterAccountID: item.MasterAccountID,
				ReshardingKey:   item.ReshardingKey,
			},
			LabelsHash:            item.LabelsHash,
			Cost:                  decimal.Decimal128(item.Cost),
			Credit:                decimal.Decimal128(item.Credit),
			CudCredit:             decimal.Decimal128(item.CudCredit),
			MonetaryGrantCredit:   decimal.Decimal128(item.MonetaryGrantCredit),
			VolumeIncentiveCredit: decimal.Decimal128(item.VolumeIncentiveCredit),
		})
	}
	return result
}
