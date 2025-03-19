package handlers

import (
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

type DumpErrorsTarget interface {
	actions.IncorrectMetricDumper
}

type DumpErrorsHandler struct {
	commonHandler
	targetFabric func() DumpErrorsTarget
}

func NewDumpErrorsHandler(
	sourceName string, handlerName string, pipeline string, targetFabric func() DumpErrorsTarget,
) *DumpErrorsHandler {
	return &DumpErrorsHandler{
		commonHandler: newCommonHandler(handlerName, sourceName, pipeline),
		targetFabric:  targetFabric,
	}
}

func (h *DumpErrorsHandler) Handle(ctx context.Context, sourceID lbtypes.SourceID, messages *lbtypes.Messages) {
	ctx, defFunc, scope := h.init(ctx, sourceID, messages)
	defer defFunc()

	if err := h.handle(ctx, scope, messages.Messages); err != nil {
		tooling.TraceTag(ctx, tracetag.Failed())
		tooling.TraceEvent(ctx, "errors dump failed", logf.Error(err))
		messages.Error(err)
		return
	}

	messages.Consumed()
}

func (h *DumpErrorsHandler) handle(
	ctx context.Context, scope entities.ProcessingScope, messages []lbtypes.ReadMessage,
) error {
	pusher := h.targetFabric()
	items, err := h.parse(ctx, messages)
	if err != nil {
		return err
	}
	tooling.IncomingMetricsCount(ctx, len(items))

	if err := actions.DumpInvalidMetrics(ctx, scope, pusher, items); err != nil {
		return err
	}
	tooling.ProcessedMetricsCount(ctx, len(items))
	return nil
}

func (h *DumpErrorsHandler) parse(ctx context.Context, msg []lbtypes.ReadMessage) (items []entities.IncorrectMetricDump, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
	}()

	for _, m := range msg {
		parsed, err := h.parseMessage(m)
		if err != nil {
			return nil, err
		}
		items = append(items, h.makeEntity(parsed)...)
	}
	tooling.TraceEvent(ctx, "parsed items", logf.Size(len(items)))
	return
}

func (*DumpErrorsHandler) parseMessage(m lbtypes.ReadMessage) (parsed []types.QueueError, err error) {
	dec := json.NewDecoder(m.DataReader)

	for dec.More() {
		var sm types.QueueError
		if err = dec.Decode(&sm); err != nil {
			return nil, fmt.Errorf("source message parse: %w", err)
		}

		parsed = append(parsed, sm)
	}
	return
}

func (*DumpErrorsHandler) makeEntity(items []types.QueueError) []entities.IncorrectMetricDump {
	if len(items) == 0 {
		return nil
	}

	result := make([]entities.IncorrectMetricDump, len(items))
	for i, item := range items {
		result[i].SequenceID = uint(item.SequenceID)
		result[i].MetricID = item.MetricID
		result[i].Reason = item.Reason
		result[i].ReasonComment = item.ReasonComment
		result[i].MetricSourceID = item.MetricSourceID
		result[i].SourceID = item.SourceID
		result[i].SourceName = item.SourceName
		result[i].Hostname = item.Hostname
		result[i].UploadedAt = item.UploadedAt.Time()
		result[i].MetricSchema = item.MetricSchema
		result[i].MetricResourceID = item.MetricResourceID
		result[i].MetricData = item.Metric
		result[i].RawMetric = item.RawMetric
	}
	return result
}
