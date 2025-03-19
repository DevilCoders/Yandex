package clickhouse

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/hashring"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/clickhouse/usage"
)

// PushIncorrectDump adds an invalid metric to the temporary buffer before flush
func (s *Session) PushIncorrectDump(
	ctx context.Context, _ entities.ProcessingScope, d entities.IncorrectMetricDump,
) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}
	if s.invalidBufferForPartition == nil {
		s.invalidBufferForPartition = make([]invalidStore, s.adapter.numOfPartitions)
	}

	row := usage.InvalidMetricRow{
		SourceName:       d.SourceName,
		Reason:           d.Reason,
		SourceID:         d.SourceID,
		ReasonComment:    d.ReasonComment,
		Hostname:         d.Hostname,
		SequenceID:       d.SequenceID,
		MetricID:         d.MetricID,
		MetricSchema:     d.MetricSchema,
		MetricSourceID:   d.MetricSourceID,
		MetricResourceID: d.MetricResourceID,
		Metric:           d.MetricData,
		RawMetric:        d.RawMetric,
	}
	// each partition at buffer corresponds some shard in Clickhouse cluster
	partition := s.adapter.splitter.Partition(row.MetricID)
	s.invalidBufferForPartition[partition].rows = append(s.invalidBufferForPartition[partition].rows, row)
	return nil
}

// FlushIncorrectDumps inserts data from buffer to Clickhouse shards
func (s *Session) FlushIncorrectDumps(ctx context.Context) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}
	return s.flushInvalidStore(ctx)
}

func (s *Session) flushInvalidStore(ctx context.Context) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}

	eg, ctx := errgroup.WithContext(ctx)
	for partition, invalidBuffer := range s.invalidBufferForPartition {
		partition := partition
		invalidBuffer := invalidBuffer

		eg.Go(func() error {
			retryCtx := tooling.StartRetry(ctx)

			err := s.retryInsert(retryCtx, func() (resErr error) {
				tooling.RetryIteration(retryCtx)

				connection, err := s.adapter.connections.DB(partition)
				if err != nil {
					return ErrConnection.Wrap(err)
				}

				queries := usage.New(connection)
				err = queries.PushInvalid(retryCtx, invalidBuffer.rows...)
				if err != nil {
					return ErrInvalidPush.Wrap(err)
				}
				invalidBuffer.rows = invalidBuffer.rows[:0]
				return nil
			})

			return err
		})
	}

	return eg.Wait()
}

type invalidStore struct {
	rows []usage.InvalidMetricRow
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

func (h *hashRing) Partition(key string) int {
	part, _ := h.ring.GetPartition(key)
	return part
}

type dummySplitter struct{}

func (dummySplitter) Partition(_ string) int { return 0 }

type splitter interface {
	Partition(string) int
}
