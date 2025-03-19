package logbroker

import (
	"context"
	"fmt"
	"sort"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types/marshal"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

func (s *Session) PushInvalidMetric(ctx context.Context, scope entities.ProcessingScope, metric entities.InvalidMetric) error {
	s.scope = scope
	data, err := marshal.InvalidMetricJSON(scope, metric)
	if err != nil {
		return err
	}

	w := &invalidMessageWrapper{
		data:   data,
		offset: metric.Offset(),
	}
	s.invalidMetrics = append(s.invalidMetrics, w)
	return nil
}

func (s *Session) FlushInvalidMetrics(ctx context.Context) (err error) {
	if err := s.adapter.inflyLimiter.Acquire(ctx, len(s.invalidMetrics)); err != nil {
		return ErrRateLimited.Wrap(err)
	}
	defer s.adapter.inflyLimiter.Release(len(s.invalidMetrics))

	ctx = tooling.ICRequestStarted(ctx, systemName)
	defer func() {
		tooling.ICRequestDone(ctx, err)
	}()
	logger := tooling.Logger(ctx)
	if len(s.invalidMetrics) == 0 {
		logger.Debug("No invalid messages to write")
		return nil
	}

	messages := s.invalidMetrics

	tooling.TraceTag(ctx, tracetag.PushMetricsCount(len(messages)))

	sort.Slice(messages, func(i, j int) bool { return messages[i].Offset() < messages[j].Offset() })
	logger.Info("invalid metrics prepared for write", logf.Size(len(messages)))

	srcID := lbtypes.SourceID(s.scope.SourceID)

	writer := s.adapter.writers.IncorrectMetrics
	if _, err := writer.Write(ctx, srcID, 0, messages); err != nil {
		return ErrWrite.Wrap(err)
	}
	logger.Debug("invalid metrics written")
	s.invalidMetrics = nil
	return nil
}

func (s *Session) EnrichedMetricPartitions() int {
	return int(s.adapter.writers.ReshardedMetrics.PartitionsCount())
}

func (s *Session) PushEnrichedMetricToPartition(
	ctx context.Context, scope entities.ProcessingScope, metric entities.EnrichedMetric, partition int,
) error {
	partitions := s.adapter.writers.ReshardedMetrics.PartitionsCount()
	if partition < 0 || partition >= int(partitions) {
		return ErrNoSuchPartition.Wrap(fmt.Errorf("%d", partition))
	}
	if len(s.enrichedMetrics) == 0 {
		s.enrichedMetrics = make([][]lbtypes.ShardMessage, partitions)
	}

	s.scope = scope
	s.enrichedMetrics[partition] = append(s.enrichedMetrics[partition], metricMessageWrapper{metric})
	return nil
}

func (s *Session) FlushEnriched(ctx context.Context) (err error) {
	logger := tooling.Logger(ctx)
	if len(s.enrichedMetrics) == 0 {
		logger.Debug("No messages to write")
		return nil
	}

	srcID := lbtypes.SourceID(s.scope.SourceID)
	eg, ctx := errgroup.WithContext(ctx)

	// NOTE: in case of OOM only this shows what happend because write is eats most memory
	msgSize := 0
	for _, messages := range s.enrichedMetrics {
		msgSize += len(messages)
	}
	logger.Info("flush metrics", logf.Size(msgSize))

	for partition := range s.enrichedMetrics {
		partition := partition
		messages := s.enrichedMetrics[partition]
		if len(messages) == 0 {
			continue
		}
		eg.Go(func() error {
			retryCtx := tooling.StartRetry(ctx)
			err := s.retryPush(retryCtx, func() error {
				tooling.RetryIteration(retryCtx)
				return s.flushMetrics(ctx, "FlushEnriched", s.adapter.writers.ReshardedMetrics, partition, srcID, messages)
			})
			if err != nil {
				return err
			}
			s.enrichedMetrics[partition] = nil
			return nil
		})
	}

	return eg.Wait()
}

func (s *Session) flushMetrics(
	ctx context.Context, name string, writer lbtypes.ShardWriter, partition int, srcID lbtypes.SourceID,
	messages []lbtypes.ShardMessage,
) (err error) {
	if err := s.adapter.inflyLimiter.Acquire(ctx, len(messages)); err != nil {
		return ErrRateLimited.Wrap(err)
	}
	defer s.adapter.inflyLimiter.Release(len(messages))
	ctx = tooling.ICRequestWithNameStarted(ctx, systemName, name)
	defer func() {
		tooling.ICRequestDone(ctx, err)
	}()

	tooling.TraceTag(ctx, tracetag.Partition(partition), tracetag.PushMetricsCount(len(messages)))
	logger := tooling.Logger(ctx)
	partitionField := logf.Partition(partition)

	sort.Slice(messages, func(i, j int) bool { return messages[i].Offset() < messages[j].Offset() })
	logger.Debug("metrics prepared for write", partitionField, logf.Size(len(messages)))

	offset, err := writer.Write(ctx, srcID, uint32(partition), messages)
	if err != nil {
		return ErrWrite.Wrap(fmt.Errorf("partition %d: %w", partition, err))
	}
	if offset != 0 {
		tooling.TraceTag(ctx, tracetag.Offset(offset))
		logger.Info("metrics written", partitionField, logf.Offset(offset))
	} else {
		logger.Warn("metrics skipped by logbroker", partitionField)
	}
	return nil
}

type metricMessageWrapper struct {
	entities.Metric
}

func (w metricMessageWrapper) Data() ([]byte, error) {
	data, err := marshal.MetricJSON(w.Metric)
	if err != nil {
		return nil, err
	}

	return []byte(data), nil
}

type invalidMessageWrapper struct {
	data   types.JSONAnything
	offset uint64
}

func (w *invalidMessageWrapper) Data() ([]byte, error) {
	return []byte(w.data), nil
}

func (w *invalidMessageWrapper) Offset() uint64 {
	return w.offset
}
