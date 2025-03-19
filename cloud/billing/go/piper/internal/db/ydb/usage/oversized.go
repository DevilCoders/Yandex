package usage

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

func (q *Queries) PushOversized(ctx context.Context, rows ...OversizedRow) (err error) {
	ctx = tooling.QueryStarted(ctx)
	tooling.QueryWithRowsCount(ctx, len(rows))

	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	query := pushOversizedQuery.WithParams(q.qp)
	_, err = q.db.ExecContext(ctx, query, sql.Named("values", param.List()))
	err = qtool.WrapWithQuery(err, query)

	return err
}

type OversizedRow struct {
	SourceID          string          `db:"source_id"`
	SeqNo             uint            `db:"seq_no"`
	CompressedMessage []byte          `db:"compressed_message"`
	CreatedAt         ydbsql.Datetime `db:"created_at"`
}

func (r OversizedRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("source_id", ydb.UTF8Value(r.SourceID)),
		ydb.StructFieldValue("seq_no", ydb.Uint64Value(uint64(r.SeqNo))),
		ydb.StructFieldValue("compressed_message", ydb.StringValue(r.CompressedMessage)),
		ydb.StructFieldValue("created_at", r.CreatedAt.Value()),
	)
}

var (
	oversizedStructType = ydb.Struct(
		ydb.StructField("source_id", ydb.TypeUTF8),
		ydb.StructField("seq_no", ydb.TypeUint64),
		ydb.StructField("compressed_message", ydb.TypeString),
		ydb.StructField("created_at", ydb.TypeDatetime),
	)
	oversizedListType = ydb.List(oversizedStructType)

	pushOversizedQuery = qtool.Query(
		qtool.Declare("values", oversizedListType),
		qtool.ReplaceFromValues("usage/realtime/oversized_messages",
			"source_id",
			"seq_no",
			"compressed_message",
			"created_at",
		),
	)
)
