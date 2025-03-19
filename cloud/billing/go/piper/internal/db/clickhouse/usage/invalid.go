package usage

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

const insertQuery = `
		INSERT INTO invalid_metrics(source_name, reason, metric_id, hostname, source_id, reason_comment,
		                            raw_metric, metric, metric_source_id, metric_schema, metric_resource_id)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
	`

func (q *Queries) PushInvalid(ctx context.Context, rows ...InvalidMetricRow) (err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	tx, err := q.connection.Begin()
	if err != nil {
		return err
	}
	stmt, err := tx.Prepare(insertQuery)
	if err != nil {
		return err
	}

	for _, row := range rows {
		if _, err := stmt.ExecContext(ctx,
			row.SourceName,
			row.Reason,
			row.MetricID,
			row.Hostname,
			row.SourceID,
			row.ReasonComment,
			row.RawMetric,
			row.Metric,
			row.SourceID,
			row.MetricSchema,
			row.MetricResourceID); err != nil {
			return err
		}
		if err != nil {
			return err
		}
	}
	return tx.Commit()
}

type InvalidMetricRow struct {
	SourceName       string `db:"source_name"`
	UploadedAt       uint64 `db:"uploaded_at"`
	Reason           string `db:"reason"`
	MetricID         string `db:"metric_id"`
	SourceID         string `db:"source_id"`
	ReasonComment    string `db:"reason_comment"`
	Hostname         string `db:"hostname"`
	RawMetric        string `db:"raw_metric"`
	Metric           string `db:"metric"`
	MetricSchema     string `db:"metric_schema"`
	MetricSourceID   string `db:"metric_source_id"`
	MetricResourceID string `db:"metric_resource_id"`
	SequenceID       uint   `db:"sequence_id"`
}
