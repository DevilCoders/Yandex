package usage

import (
	"context"
	"database/sql"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

func (s *Schemes) CreateCumulativeLog(ctx context.Context, month time.Time) (err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	query := createCumulativeLogQuery(month).WithParams(s.qp)
	err = qtool.WrapWithQuery(
		s.db.ExecuteSchemeQuery(ctx, query),
		query,
	)
	return
}

func (s *Schemes) CreateCumulativeTracking(ctx context.Context) (err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	query := createCumulativeTrackingQuery.WithParams(s.qp)
	err = qtool.WrapWithQuery(s.db.ExecuteSchemeQuery(ctx, query), query)
	return
}

func (q *Queries) CalculateCumulative(
	ctx context.Context, month time.Time, source string, records ...CumulativeCalculateRecord,
) (count int, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	param := qtool.ListValues()
	for _, r := range records {
		param.Add(r)
	}
	monthParam := fmt.Sprintf("%s-01", month.Format("2006-01"))
	query := calculateCumulativeQuery(month).WithParams(q.qp)

	var result dummyCountRow
	err = q.db.GetContext(ctx, &result, query,
		sql.Named("source_id", ydb.UTF8Value(source)),
		sql.Named("month", ydb.UTF8Value(monthParam)),
		sql.Named("records", param.List()),
	)
	err = qtool.WrapWithQuery(err, query)
	count = int(result.Count)
	return
}

func (q *Queries) GetCumulativeLogByOffset(
	ctx context.Context, month time.Time, source string, from, to, limit uint64, cursor CumulativeLogCursor,
) (result []CumulativeLogRow, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	var cursorVal ydb.Value
	switch {
	case cursor == CumulativeLogCursor{}:
		cursorVal = ydb.NullValue(cumulativeCursorStruct)
	default:
		cursorVal = ydb.OptionalValue(ydb.StructValue(
			ydb.StructFieldValue("sequence_id", ydb.Uint64Value(cursor.SequenceID)),
			ydb.StructFieldValue("resource_id", ydb.UTF8Value(cursor.ResourceID)),
			ydb.StructFieldValue("sku_id", ydb.UTF8Value(cursor.SkuID)),
		))
	}

	query := getCumulativeLogByOffset(month).WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query,
		sql.Named("source_id", ydb.UTF8Value(source)),
		sql.Named("start_no", ydb.Uint64Value(from)),
		sql.Named("end_no", ydb.Uint64Value(to)),
		sql.Named("limit", ydb.Uint64Value(limit)),
		sql.Named("cursor", cursorVal),
	)
	err = qtool.WrapWithQuery(err, query)
	return
}

var (
	cumulativeTrackingSpec = qtool.TableSpec([]string{"resource_id", "sku_id"},
		qtool.TableCol("resource_id", ydb.TypeUTF8),
		qtool.TableCol("sku_id", ydb.TypeUTF8),
		qtool.TableCol("first_use_month", ydb.TypeUTF8),
	)
	cumulativeUsageSpec = qtool.TableSpec([]string{"resource_id", "sku_id"},
		qtool.TableCol("resource_id", ydb.TypeUTF8),
		qtool.TableCol("sku_id", ydb.TypeUTF8),
		qtool.TableCol("quantity", ydb.Decimal(22, 9)),
	)
	cumulativeLogsSpec = qtool.TableSpec([]string{"source_id", "sequence_id", "resource_id", "sku_id"},
		qtool.TableCol("resource_id", ydb.TypeUTF8),
		qtool.TableCol("source_id", ydb.TypeUTF8),
		qtool.TableCol("sku_id", ydb.TypeUTF8),
		qtool.TableCol("first_use_month", ydb.TypeUTF8),
		qtool.TableCol("sequence_id", ydb.TypeUint64),
		qtool.TableCol("quantity", ydb.Decimal(22, 9)),
		qtool.TableCol("delta", ydb.Decimal(22, 9)),
	)

	createCumulativeTrackingQuery = qtool.Query(
		"CREATE TABLE", qtool.Table("usage/realtime/cumulative/global/resources"), "(", cumulativeTrackingSpec, ");",
	)
)

func createCumulativeLogQuery(month time.Time) qtool.Template {
	monthStr := month.Format("2006-01")
	return qtool.Query(
		"CREATE TABLE", qtool.Table(fmt.Sprintf("usage/realtime/cumulative/%s-01/usage", monthStr)), "(",
		cumulativeUsageSpec,
		");",

		"CREATE TABLE", qtool.Table(fmt.Sprintf("usage/realtime/cumulative/%s-01/logs", monthStr)), "(",
		cumulativeLogsSpec,
		");",
	)
}

type dummyCountRow struct {
	Count uint64 `db:"cnt"`
}

type CumulativeCalculateRecord struct {
	ResourceID string
	SequenceID uint64
	SkuID      string
	Quantity   qtool.DefaultDecimal
}

func (r CumulativeCalculateRecord) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("resource_id", ydb.UTF8Value(r.ResourceID)),
		ydb.StructFieldValue("sequence_id", ydb.Uint64Value(r.SequenceID)),
		ydb.StructFieldValue("sku_id", ydb.UTF8Value(r.SkuID)),
		ydb.StructFieldValue("quantity", r.Quantity.Value()),
	)
}

var (
	cumulativeCalculateStruct = ydb.Struct(
		ydb.StructField("resource_id", ydb.TypeUTF8),
		ydb.StructField("sequence_id", ydb.TypeUint64),
		ydb.StructField("sku_id", ydb.TypeUTF8),
		ydb.StructField("quantity", ydb.Decimal(22, 9)),
	)
	cumulativeCalculateListType = ydb.List(cumulativeCalculateStruct)
)

func calculateCumulativeQuery(month time.Time) qtool.Template {
	monthStr := month.Format("2006-01")
	usageTable := fmt.Sprintf("usage/realtime/cumulative/%s-01/usage", monthStr)
	logsTable := fmt.Sprintf("usage/realtime/cumulative/%s-01/logs", monthStr)

	return qtool.Query(
		qtool.Declare("source_id", ydb.TypeUTF8),
		qtool.Declare("month", ydb.TypeUTF8),
		qtool.Declare("records", cumulativeCalculateListType),
		qtool.DecimalZeroDecl,

		// Join with usage table to find all resources in the current month
		// and find resources with changed quantity e.g. 8 cores > 4 cores
		qtool.NamedQuery("changed",
			"SELECT", qtool.Cols(
				qtool.PrefixedCols("input",
					"sequence_id",
					"resource_id",
					"sku_id",
					"quantity",
				),
				"input.quantity - NVL(table.quantity, $zero) as delta",
			),
			"FROM as_table($records) as input",
			" LEFT JOIN", qtool.TableAs(usageTable, "table"),
			"ON input.resource_id = table.resource_id AND input.sku_id = table.sku_id",
			"WHERE table.resource_id IS NULL OR (input.quantity > table.quantity)",
		),

		// Find `first_use_month` for each changed resource
		qtool.NamedQuery("result",
			"SELECT", qtool.Cols(
				qtool.PrefixedCols("changed",
					"sequence_id",
					"resource_id",
					"sku_id",
					"quantity",
					"delta",
				),
				"NVL(tracking.first_use_month, $month) as first_use_month",
				"IF(tracking.resource_id IS NULL, 0, 1) as has_resource",
			),
			"FROM $changed as changed",
			"  LEFT JOIN", qtool.TableAs("usage/realtime/cumulative/global/resources", "tracking"), "ON",
			"    changed.resource_id = tracking.resource_id AND",
			"    changed.sku_id = tracking.sku_id",
		),

		// Save changes into tables
		"UPSERT INTO", qtool.Table(usageTable),
		"SELECT", qtool.Cols("resource_id", "sku_id", "quantity"),
		"FROM $result;",

		"UPSERT INTO", qtool.Table(logsTable),
		"SELECT", qtool.Cols(
			"sequence_id", "resource_id", "sku_id", "quantity", "delta", "first_use_month", "$source_id as source_id",
		),
		"FROM $result;",

		"UPSERT INTO", qtool.Table("usage/realtime/cumulative/global/resources"),
		"SELECT", qtool.Cols("resource_id", "sku_id", "first_use_month"),
		"FROM $result",
		"WHERE has_resource = 0;",

		// Report diff size
		"SELECT count(*) as cnt FROM $result;",
	)
}

type CumulativeLogRow struct {
	ResourceID    string               `db:"resource_id"`
	SourceID      string               `db:"source_id"`
	SkuID         string               `db:"sku_id"`
	FirstUseMonth string               `db:"first_use_month"`
	SequenceID    uint64               `db:"sequence_id"`
	Quantity      qtool.DefaultDecimal `db:"quantity"`
	Delta         qtool.DefaultDecimal `db:"delta"`
}

type CumulativeLogCursor struct {
	SequenceID uint64
	ResourceID string
	SkuID      string
}

var cumulativeCursorStruct = ydb.Struct(
	ydb.StructField("sequence_id", ydb.TypeUint64),
	ydb.StructField("resource_id", ydb.TypeUTF8),
	ydb.StructField("sku_id", ydb.TypeUTF8),
)

func getCumulativeLogByOffset(month time.Time) qtool.Template {
	monthStr := month.Format("2006-01")
	logsTable := fmt.Sprintf("usage/realtime/cumulative/%s-01/logs", monthStr)

	return qtool.Query(
		qtool.Declare("source_id", ydb.TypeUTF8),
		qtool.Declare("start_no", ydb.TypeUint64),
		qtool.Declare("end_no", ydb.TypeUint64),
		qtool.Declare("limit", ydb.TypeUint64),

		qtool.Declare("cursor", ydb.Optional(cumulativeCursorStruct)),

		"SELECT", qtool.Cols("resource_id", "source_id", "sku_id", "first_use_month", "sequence_id", "quantity", "delta"),
		"FROM", qtool.Table(logsTable),
		"WHERE source_id = $source_id AND sequence_id BETWEEN $start_no AND $end_no",
		" AND ($cursor IS NULL OR (sequence_id, resource_id, sku_id) > ($cursor.sequence_id, $cursor.resource_id, $cursor.sku_id))",
		"ORDER BY source_id, sequence_id, resource_id, sku_id",
		"LIMIT $limit;",
	)
}
