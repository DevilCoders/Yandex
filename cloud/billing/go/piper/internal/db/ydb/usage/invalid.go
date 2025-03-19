package usage

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

func (q *Queries) PushInvalid(ctx context.Context, rows ...InvalidMetricRow) (err error) {
	ctx = tooling.QueryStarted(ctx)
	tooling.QueryWithRowsCount(ctx, len(rows))
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	query := pushInvalidQuery.WithParams(q.qp)
	_, err = q.db.ExecContext(ctx, query, sql.Named("values", param.List()))
	err = qtool.WrapWithQuery(err, query)

	return err
}

type InvalidMetricRow struct {
	SourceName       string             `db:"source_name"`
	UploadedAt       qtool.UInt64Ts     `db:"uploaded_at"`
	Reason           string             `db:"reason"`
	MetricID         string             `db:"metric_id"`
	SourceID         string             `db:"source_id"`
	ReasonComment    string             `db:"reason_comment"`
	Hostname         string             `db:"hostname"`
	RawMetric        string             `db:"raw_metric"`
	Metric           qtool.JSONAnything `db:"metric"`
	MetricSchema     string             `db:"metric_schema"`
	MetricSourceID   string             `db:"metric_source_id"`
	MetricResourceID string             `db:"metric_resource_id"`
	SequenceID       uint               `db:"sequence_id"`
}

func (r InvalidMetricRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("source_name", ydb.UTF8Value(r.SourceName)),
		ydb.StructFieldValue("uploaded_at", r.UploadedAt.Value()),
		ydb.StructFieldValue("reason", ydb.UTF8Value(r.Reason)),
		ydb.StructFieldValue("metric_id", ydb.UTF8Value(r.MetricID)),
		ydb.StructFieldValue("source_id", ydb.UTF8Value(r.SourceID)),
		ydb.StructFieldValue("reason_comment", ydb.UTF8Value(r.ReasonComment)),
		ydb.StructFieldValue("hostname", ydb.UTF8Value(r.Hostname)),
		ydb.StructFieldValue("raw_metric", ydb.UTF8Value(r.RawMetric)),
		ydb.StructFieldValue("metric", r.Metric.Value()),
		ydb.StructFieldValue("metric_schema", ydb.UTF8Value(r.MetricSchema)),
		ydb.StructFieldValue("metric_source_id", ydb.UTF8Value(r.MetricSourceID)),
		ydb.StructFieldValue("metric_resource_id", ydb.UTF8Value(r.MetricResourceID)),
		ydb.StructFieldValue("sequence_id", ydb.Uint64Value(uint64(r.SequenceID))),
	)
}

var (
	invalidStructType = ydb.Struct(
		ydb.StructField("source_name", ydb.TypeUTF8),
		ydb.StructField("uploaded_at", ydb.TypeUint64),
		ydb.StructField("reason", ydb.TypeUTF8),
		ydb.StructField("metric_id", ydb.TypeUTF8),
		ydb.StructField("source_id", ydb.TypeUTF8),
		ydb.StructField("reason_comment", ydb.TypeUTF8),
		ydb.StructField("hostname", ydb.TypeUTF8),
		ydb.StructField("raw_metric", ydb.TypeUTF8),
		ydb.StructField("metric", ydb.Optional(ydb.TypeJSON)),
		ydb.StructField("metric_schema", ydb.TypeUTF8),
		ydb.StructField("metric_source_id", ydb.TypeUTF8),
		ydb.StructField("metric_resource_id", ydb.TypeUTF8),
		ydb.StructField("sequence_id", ydb.TypeUint64),
	)
	invalidListType = ydb.List(invalidStructType)

	pushInvalidQuery = qtool.Query(
		qtool.Declare("values", invalidListType),
		qtool.ReplaceFromValues("usage/realtime/invalid_metrics",
			"source_name",
			"uploaded_at",
			"reason",
			"metric_id",
			"source_id",
			"reason_comment",
			"hostname",
			"raw_metric",
			"metric",
			"metric_schema",
			"metric_source_id",
			"metric_resource_id",
			"sequence_id",
		),
	)
)
