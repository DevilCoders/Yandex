package actions

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/hashring"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
)

type InvalidMetricsPusher interface {
	PushInvalidMetric(context.Context, entities.ProcessingScope, entities.InvalidMetric) error
	FlushInvalidMetrics(context.Context) error
}

type MetricsPusher interface {
	PushEnrichedMetricToPartition(context.Context, entities.ProcessingScope, entities.EnrichedMetric, int) error
	EnrichedMetricPartitions() int
	FlushEnriched(context.Context) error
}

type OversizedMessagePusher interface {
	PushOversizedMessage(context.Context, entities.ProcessingScope, entities.IncorrectRawMessage) error
	FlushOversized(context.Context) error
}

type IncorrectMetricDumper interface {
	PushIncorrectDump(context.Context, entities.ProcessingScope, entities.IncorrectMetricDump) error
	FlushIncorrectDumps(context.Context) error
}

func PushOversizedMessages(
	ctx context.Context, scope entities.ProcessingScope, pusher OversizedMessagePusher,
	messages []entities.InvalidMetric,
) (err error) {
	ctx = tooling.ActionStarted(ctx)
	// NOTE: invalid metrics counting should be outside this action
	defer func() { tooling.ActionDone(ctx, err) }()

	if len(messages) == 0 {
		return
	}

	logger := tooling.Logger(ctx)
	for _, m := range messages {
		if err = pusher.PushOversizedMessage(ctx, scope, m.IncorrectRawMessage); err != nil {
			logger.Error(
				"push message error", logf.Error(err), logf.Offset(m.MessageOffset),
			)
			return err
		}
		logger.Debug(
			"too large message", logf.Size(len(m.RawMetric)), logf.Offset(m.MessageOffset),
		)
	}

	return pusher.FlushOversized(ctx)
}

func PushEnrichedMetrics(
	ctx context.Context, scope entities.ProcessingScope, pusher MetricsPusher,
	metrics []entities.EnrichedMetric,
) (err error) {
	ctx = tooling.ActionStarted(ctx)
	// NOTE: pushed metrics counting should be placed in pusher
	defer func() { tooling.ActionDone(ctx, err) }()

	splitter := newSplitter(pusher.EnrichedMetricPartitions())
	for _, m := range metrics {
		if err = pusher.PushEnrichedMetricToPartition(ctx, scope, m, splitter.partition(m.ReshardingKey)); err != nil {
			return
		}
	}

	return pusher.FlushEnriched(ctx)
}

func PushInvalidMetrics(
	ctx context.Context, scope entities.ProcessingScope, pusher InvalidMetricsPusher,
	metrics []entities.InvalidMetric,
) (err error) {
	ctx = tooling.ActionStarted(ctx)
	// NOTE: pushed metrics counting should be placed in pusher
	defer func() { tooling.ActionDone(ctx, err) }()

	for _, m := range metrics {
		if err = pusher.PushInvalidMetric(ctx, scope, m); err != nil {
			return
		}
	}

	return pusher.FlushInvalidMetrics(ctx)
}

func DumpInvalidMetrics(
	ctx context.Context, scope entities.ProcessingScope, pusher IncorrectMetricDumper,
	items []entities.IncorrectMetricDump,
) (err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() { tooling.ActionDone(ctx, err) }()

	for _, item := range items {
		if err = pusher.PushIncorrectDump(ctx, scope, item); err != nil {
			return
		}
	}

	return pusher.FlushIncorrectDumps(ctx)
}

func newSplitter(partitions int) splitter {
	if partitions <= 1 {
		return dummySplitter{}
	}

	const ( // Hi to custom python implementation
		nodePrefix = "realtime"
	)
	return hashRings.getRing(nodePrefix, partitions)
}

type hashRing struct {
	ring *hashring.CompatibleHashring
}

func (h *hashRing) partition(key string) int {
	part, _ := h.ring.GetPartition(key)
	return part
}

type dummySplitter struct{}

func (dummySplitter) partition(_ string) int { return 0 }

type splitter interface {
	partition(string) int
}
