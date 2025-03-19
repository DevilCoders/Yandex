package ydb

import (
	"bytes"
	"compress/zlib"
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/usage"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types/marshal"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

func (s *MetaSession) PushInvalidMetric(ctx context.Context, scope entities.ProcessingScope, mtr entities.InvalidMetric) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}

	metricJSON, err := marshal.MetricJSON(mtr.Metric)
	if err != nil {
		return err
	}
	metricIDs := mtr.MetricID()

	row := usage.InvalidMetricRow{
		SourceName:    scope.SourceName,
		UploadedAt:    qtool.UInt64Ts(mtr.UploadedAt()),
		Reason:        mtr.Reason.String(),
		SourceID:      fmt.Sprintf("%s:%s", scope.SourceType, scope.SourceID),
		ReasonComment: mtr.ReasonComment,
		Hostname:      scope.Hostname,
		SequenceID:    uint(mtr.MessageOffset) + 1,

		MetricID:         metricIDs.ID,
		MetricSchema:     metricIDs.Schema,
		MetricSourceID:   metricIDs.SourceID,
		MetricResourceID: metricIDs.ResourceID,
		RawMetric:        string(mtr.RawMetric),
		Metric:           qtool.JSONAnything(metricJSON),
	}
	s.invalidBuffer.rows = append(s.invalidBuffer.rows, row)
	return nil
}

func (s *MetaSession) FlushInvalidMetrics(ctx context.Context) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}
	if len(s.invalidBuffer.rows) == 0 {
		return nil
	}
	return s.flushInvalidStore(ctx)
}

func (s *MetaSession) PushOversizedMessage(
	ctx context.Context, scope entities.ProcessingScope, msg entities.IncorrectRawMessage,
) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}
	row := usage.OversizedRow{
		SourceID:          fmt.Sprintf("%s:%s", scope.SourceType, scope.SourceID),
		SeqNo:             uint(msg.MessageOffset) + 1,
		CompressedMessage: compressMessage(msg.RawMetric),
		CreatedAt:         ydbsql.Datetime(msg.UploadedAt()),
	}
	s.oversizedBuffer.rows = append(s.oversizedBuffer.rows, row)
	return nil
}

func (s *MetaSession) FlushOversized(ctx context.Context) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}
	if len(s.oversizedBuffer.rows) == 0 {
		return nil
	}

	queries := usage.New(s.adapter.db, s.adapter.queryParams)
	err := queries.PushOversized(ctx, s.oversizedBuffer.rows...)
	if err != nil {
		return ErrOversizedPush.Wrap(err)
	}
	s.oversizedBuffer.rows = s.oversizedBuffer.rows[:0]
	return nil
}

func (s *MetaSession) PushIncorrectDump(
	ctx context.Context, _ entities.ProcessingScope, d entities.IncorrectMetricDump,
) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}

	row := usage.InvalidMetricRow{
		SourceName:    d.SourceName,
		UploadedAt:    qtool.UInt64Ts(d.UploadedAt),
		Reason:        d.Reason,
		SourceID:      d.SourceID,
		ReasonComment: d.ReasonComment,
		Hostname:      d.Hostname,
		SequenceID:    d.SequenceID,

		MetricID:         d.MetricID,
		MetricSchema:     d.MetricSchema,
		MetricSourceID:   d.MetricSourceID,
		MetricResourceID: d.MetricResourceID,
		Metric:           qtool.JSONAnything(d.MetricData),
		RawMetric:        d.RawMetric,
	}
	s.invalidBuffer.rows = append(s.invalidBuffer.rows, row)
	return nil
}

func (s *MetaSession) FlushIncorrectDumps(ctx context.Context) error {
	if ctx.Err() != nil {
		return ctx.Err()
	}
	if len(s.invalidBuffer.rows) == 0 {
		return nil
	}
	return s.flushInvalidStore(ctx)
}

func (s *MetaSession) flushInvalidStore(ctx context.Context) error {
	queries := usage.New(s.adapter.db, s.adapter.queryParams)
	err := queries.PushInvalid(ctx, s.invalidBuffer.rows...)
	if err != nil {
		return ErrInvalidPush.Wrap(err)
	}
	s.invalidBuffer.rows = s.invalidBuffer.rows[:0]
	return nil
}

type oversizedStore struct {
	rows []usage.OversizedRow
}

type invalidStore struct {
	rows []usage.InvalidMetricRow
}

func compressMessage(data []byte) []byte {
	buf := bytes.Buffer{}
	wr := zlib.NewWriter(&buf)
	_, _ = wr.Write(data)
	_ = wr.Close()
	return buf.Bytes()
}
