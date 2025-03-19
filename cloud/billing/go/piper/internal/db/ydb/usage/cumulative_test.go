package usage

import (
	"context"
	"database/sql"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type cumulativeSchemeTestSuite struct {
	baseSuite
}

func TestCumulativeScheme(t *testing.T) {
	suite.Run(t, new(cumulativeSchemeTestSuite))
}

func (suite *cumulativeSchemeTestSuite) TestTableCompliance() {
	month := time.Unix(0, 0)
	err := suite.scheme.CreateCumulativeTracking(suite.ctx)
	suite.Require().NoError(err)
	err = suite.scheme.CreateCumulativeLog(suite.ctx, month)
	suite.Require().NoError(err)

	err = suite.queries.pushCumulativeTracking(suite.ctx, cumulativeTrackRow{})
	suite.Require().NoError(err)

	err = suite.queries.pushCumulativeUsage(suite.ctx, month, cumulativeUsageRow{})
	suite.Require().NoError(err)

	err = suite.queries.pushCumulativeLogs(suite.ctx, month, CumulativeLogRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.getAllCumulativeTracking(suite.ctx)
	suite.Require().NoError(err)

	_, err = suite.queries.getAllCumulativeUsage(suite.ctx, month)
	suite.Require().NoError(err)

	_, err = suite.queries.getAllCumulativeLogs(suite.ctx, month)
	suite.Require().NoError(err)

	err = suite.scheme.dropCumulativeLog(suite.ctx, month)
	suite.Require().NoError(err)
}

type cumulativeTestSuite struct {
	baseSuite

	month time.Time
}

func TestCumulative(t *testing.T) {
	suite.Run(t, new(cumulativeTestSuite))
}

func (suite *cumulativeTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()
	suite.month = time.Date(2000, 6, 15, 0, 0, 0, 0, time.UTC)

	_ = suite.scheme.dropCumulativeLog(suite.ctx, suite.month)
	_ = suite.scheme.dropCumulativeTracking(suite.ctx)

	err := suite.scheme.CreateCumulativeTracking(suite.ctx)
	suite.Require().NoError(err)
	err = suite.scheme.CreateCumulativeLog(suite.ctx, suite.month)
	suite.Require().NoError(err)
}

func (suite *cumulativeTestSuite) TearDownTest() {
	defer suite.baseSuite.TearDownTest()

	err := suite.scheme.dropCumulativeLog(suite.ctx, suite.month)
	suite.Require().NoError(err)

	err = suite.scheme.dropCumulativeTracking(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *cumulativeTestSuite) TestGetCumulativeLogByOffset() {
	rows := []CumulativeLogRow{
		{SourceID: "src", SequenceID: 1, ResourceID: "res1", SkuID: "sku1", FirstUseMonth: "2000-06-01"},
		{SourceID: "src", SequenceID: 1, ResourceID: "res1", SkuID: "sku2", FirstUseMonth: "2000-06-01"},
		{SourceID: "src", SequenceID: 1, ResourceID: "res2", SkuID: "sku1", FirstUseMonth: "2000-06-01"},
		{SourceID: "src", SequenceID: 1, ResourceID: "res2", SkuID: "sku2", FirstUseMonth: "2000-06-01"},
		{SourceID: "src", SequenceID: 2, ResourceID: "res1", SkuID: "sku1", FirstUseMonth: "2000-06-01"},
		{SourceID: "src", SequenceID: 2, ResourceID: "res1", SkuID: "sku2", FirstUseMonth: "2000-06-01"},
		{SourceID: "src", SequenceID: 2, ResourceID: "res2", SkuID: "sku1", FirstUseMonth: "2000-06-01"},
		{SourceID: "src", SequenceID: 2, ResourceID: "res2", SkuID: "sku2", FirstUseMonth: "2000-06-01"},
		{SourceID: "other"},
	}

	err := suite.queries.pushCumulativeLogs(suite.ctx, suite.month, rows...)
	suite.Require().NoError(err)

	got, err := suite.queries.GetCumulativeLogByOffset(suite.ctx, suite.month, "src",
		1, 2, 10, CumulativeLogCursor{},
	)
	suite.Require().NoError(err)
	suite.ElementsMatch(rows[:8], got)

	var cursor CumulativeLogCursor
	pos := 0
	step := 1
	for {
		iter, err := suite.queries.GetCumulativeLogByOffset(suite.ctx, suite.month, "src", 1, 2, uint64(step), cursor)
		suite.Require().NoError(err)
		if len(iter) == 0 {
			break
		}

		suite.Require().ElementsMatch(rows[pos:pos+step], iter)

		pos += step
		lastRow := iter[len(iter)-1]
		cursor = CumulativeLogCursor{
			SequenceID: lastRow.SequenceID,
			ResourceID: lastRow.ResourceID,
			SkuID:      lastRow.SkuID,
		}
	}
	suite.Require().Equal(8, pos)
}

func (suite *cumulativeTestSuite) TestCalculateFromScratch() {
	records := []CumulativeCalculateRecord{
		{SequenceID: 1, ResourceID: "rs1", SkuID: "sku1.1", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(1)))},
		{SequenceID: 1, ResourceID: "rs1", SkuID: "sku1.2", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(2)))},
		{SequenceID: 1, ResourceID: "rs2", SkuID: "sku2", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(10)))},
	}

	cnt, err := suite.queries.CalculateCumulative(suite.ctx, suite.month, "src", records...)
	suite.Require().NoError(err)
	suite.EqualValues(3, cnt)

	tracks, err := suite.queries.getAllCumulativeTracking(suite.ctx)
	suite.Require().NoError(err)
	usage, err := suite.queries.getAllCumulativeUsage(suite.ctx, suite.month)
	suite.Require().NoError(err)
	logs, err := suite.queries.getAllCumulativeLogs(suite.ctx, suite.month)
	suite.Require().NoError(err)

	wantTracks := []cumulativeTrackRow{
		{ResourceID: "rs1", SkuID: "sku1.1", FirstUseMonth: "2000-06-01"},
		{ResourceID: "rs1", SkuID: "sku1.2", FirstUseMonth: "2000-06-01"},
		{ResourceID: "rs2", SkuID: "sku2", FirstUseMonth: "2000-06-01"},
	}
	suite.Require().ElementsMatch(wantTracks, tracks)

	wantUsage := []cumulativeUsageRow{
		{ResourceID: "rs1", SkuID: "sku1.1", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(1)))},
		{ResourceID: "rs1", SkuID: "sku1.2", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(2)))},
		{ResourceID: "rs2", SkuID: "sku2", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(10)))},
	}
	suite.Require().ElementsMatch(wantUsage, usage)

	wantLogs := []CumulativeLogRow{
		{
			SequenceID: 1, ResourceID: "rs1", SkuID: "sku1.1", SourceID: "src", FirstUseMonth: "2000-06-01",
			Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(1))),
			Delta:    qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(1))),
		},
		{
			SequenceID: 1, ResourceID: "rs1", SkuID: "sku1.2", SourceID: "src", FirstUseMonth: "2000-06-01",
			Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(2))),
			Delta:    qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(2))),
		},
		{
			SequenceID: 1, ResourceID: "rs2", SkuID: "sku2", SourceID: "src", FirstUseMonth: "2000-06-01",
			Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(10))),
			Delta:    qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(10))),
		},
	}
	suite.Require().ElementsMatch(wantLogs, logs)
}

func (suite *cumulativeTestSuite) TestCalculateTwice() {
	records := []CumulativeCalculateRecord{
		{SequenceID: 1, ResourceID: "rs1", SkuID: "sku1.1", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(1)))},
		{SequenceID: 1, ResourceID: "rs1", SkuID: "sku1.2", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(2)))},
		{SequenceID: 1, ResourceID: "rs2", SkuID: "sku2", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(10)))},
	}

	cnt, err := suite.queries.CalculateCumulative(suite.ctx, suite.month, "src", records...)
	suite.Require().NoError(err)
	suite.EqualValues(3, cnt)

	cnt, err = suite.queries.CalculateCumulative(suite.ctx, suite.month, "src", records...)
	suite.Require().NoError(err)
	suite.Zero(cnt)
}

func (suite *cumulativeTestSuite) TestCalculateDelta() {
	given := []CumulativeCalculateRecord{
		{SequenceID: 1, ResourceID: "rs", SkuID: "sku", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(1)))},
	}

	_, err := suite.queries.CalculateCumulative(suite.ctx, suite.month, "src", given...)
	suite.Require().NoError(err)

	records := []CumulativeCalculateRecord{
		{SequenceID: 99, ResourceID: "rs", SkuID: "sku", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(3)))},
	}

	cnt, err := suite.queries.CalculateCumulative(suite.ctx, suite.month, "src", records...)
	suite.Require().NoError(err)
	suite.EqualValues(1, cnt)

	log, err := suite.queries.GetCumulativeLogByOffset(suite.ctx, suite.month, "src", 99, 99, 100, CumulativeLogCursor{})
	suite.Require().NoError(err)
	suite.Require().Len(log, 1)

	quantity := decimal.Decimal128(log[0].Quantity)
	delta := decimal.Decimal128(log[0].Delta)
	suite.Zero(quantity.Cmp(decimal.Must(decimal.FromInt64(3))), quantity.String())
	suite.Zero(delta.Cmp(decimal.Must(decimal.FromInt64(2))), delta.String())
}

func (suite *cumulativeTestSuite) TestCalculateSkipLowerQuantity() {
	given := []CumulativeCalculateRecord{
		{SequenceID: 1, ResourceID: "rs", SkuID: "sku", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(10)))},
	}

	_, err := suite.queries.CalculateCumulative(suite.ctx, suite.month, "src", given...)
	suite.Require().NoError(err)

	records := []CumulativeCalculateRecord{
		{SequenceID: 99, ResourceID: "rs", SkuID: "sku", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(3)))},
	}

	cnt, err := suite.queries.CalculateCumulative(suite.ctx, suite.month, "src", records...)
	suite.Require().NoError(err)
	suite.Zero(cnt)

	log, err := suite.queries.GetCumulativeLogByOffset(suite.ctx, suite.month, "src", 99, 99, 100, CumulativeLogCursor{})
	suite.Require().NoError(err)
	suite.Require().Empty(log)
}

func (suite *cumulativeTestSuite) TestCalculateFirstMonth() {
	err := suite.queries.pushCumulativeTracking(suite.ctx, []cumulativeTrackRow{
		{ResourceID: "rs", SkuID: "sku", FirstUseMonth: "1970-12-01"},
	}...)
	suite.Require().NoError(err)

	records := []CumulativeCalculateRecord{
		{SequenceID: 99, ResourceID: "rs", SkuID: "sku", Quantity: qtool.DefaultDecimal(decimal.Must(decimal.FromInt64(3)))},
	}

	_, err = suite.queries.CalculateCumulative(suite.ctx, suite.month, "src", records...)
	suite.Require().NoError(err)

	log, err := suite.queries.GetCumulativeLogByOffset(suite.ctx, suite.month, "src", 99, 99, 100, CumulativeLogCursor{})
	suite.Require().NoError(err)
	suite.Require().Len(log, 1)

	suite.EqualValues("1970-12-01", log[0].FirstUseMonth)
}

func (s *Schemes) dropCumulativeLog(ctx context.Context, month time.Time) (err error) {
	query := dropCumulativeLogQuery(month).WithParams(s.qp)
	err = s.db.ExecuteSchemeQuery(ctx, query)
	return
}

func (s *Schemes) dropCumulativeTracking(ctx context.Context) (err error) {
	query := qtool.Query(
		"DROP TABLE", qtool.Table("usage/realtime/cumulative/global/resources"),
	).WithParams(s.qp)
	err = qtool.WrapWithQuery(
		s.db.ExecuteSchemeQuery(ctx, query),
		query,
	)
	return
}

func (q *Queries) pushCumulativeTracking(ctx context.Context, rows ...cumulativeTrackRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}

	_, err = q.db.ExecContext(ctx, pushCumulativeTrackQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return err
}

func (q *Queries) getAllCumulativeTracking(ctx context.Context) (result []cumulativeTrackRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllCumulativeTrackQuery.WithParams(q.qp))
	return
}

func (q *Queries) pushCumulativeUsage(ctx context.Context, month time.Time, rows ...cumulativeUsageRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	query := pushCumulativeUsageQuery(month).WithParams(q.qp)
	_, err = q.db.ExecContext(ctx, query, sql.Named("values", param.List()))

	return err
}

func (q *Queries) getAllCumulativeUsage(ctx context.Context, month time.Time) (result []cumulativeUsageRow, err error) {
	query := getAllCumulativeUsageQuery(month).WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query)
	return
}

func (q *Queries) pushCumulativeLogs(ctx context.Context, month time.Time, rows ...CumulativeLogRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	query := pushCumulativeLogsQuery(month).WithParams(q.qp)
	_, err = q.db.ExecContext(ctx, query, sql.Named("values", param.List()))

	return err
}

func (q *Queries) getAllCumulativeLogs(ctx context.Context, month time.Time) (result []CumulativeLogRow, err error) {
	query := getAllCumulativeLogsQuery(month).WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query)
	return
}

func dropCumulativeLogQuery(month time.Time) qtool.Template {
	monthStr := month.Format("2006-01")
	return qtool.Query(
		"DROP TABLE", qtool.Table(fmt.Sprintf("usage/realtime/cumulative/%s-01/usage", monthStr)), ";",
		"DROP TABLE", qtool.Table(fmt.Sprintf("usage/realtime/cumulative/%s-01/logs", monthStr)), ";",
	)
}

type cumulativeTrackRow struct {
	ResourceID    string `db:"resource_id"`
	SkuID         string `db:"sku_id"`
	FirstUseMonth string `db:"first_use_month"`
}

func (r cumulativeTrackRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("resource_id", ydb.UTF8Value(r.ResourceID)),
		ydb.StructFieldValue("sku_id", ydb.UTF8Value(r.SkuID)),
		ydb.StructFieldValue("first_use_month", ydb.UTF8Value(r.FirstUseMonth)),
	)
}

var (
	cumulativeTrackStructType = ydb.Struct(
		ydb.StructField("resource_id", ydb.TypeUTF8),
		ydb.StructField("sku_id", ydb.TypeUTF8),
		ydb.StructField("first_use_month", ydb.TypeUTF8),
	)
	cumulativeTrackListType = ydb.List(cumulativeTrackStructType)

	pushCumulativeTrackQuery = qtool.Query(
		qtool.Declare("values", cumulativeTrackListType),
		qtool.ReplaceFromValues("usage/realtime/cumulative/global/resources",
			"resource_id",
			"sku_id",
			"first_use_month",
		),
	)

	getAllCumulativeTrackQuery = qtool.Query(
		"SELECT * FROM", qtool.Table("usage/realtime/cumulative/global/resources"),
	)
)

type cumulativeUsageRow struct {
	ResourceID string               `db:"resource_id"`
	SkuID      string               `db:"sku_id"`
	Quantity   qtool.DefaultDecimal `db:"quantity"`
}

func (r cumulativeUsageRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("resource_id", ydb.UTF8Value(r.ResourceID)),
		ydb.StructFieldValue("sku_id", ydb.UTF8Value(r.SkuID)),
		ydb.StructFieldValue("quantity", r.Quantity.Value()),
	)
}

var (
	cumulativeUsageStructType = ydb.Struct(
		ydb.StructField("resource_id", ydb.TypeUTF8),
		ydb.StructField("sku_id", ydb.TypeUTF8),
		ydb.StructField("quantity", ydb.Decimal(22, 9)),
	)
	cumulativeUsageListType = ydb.List(cumulativeUsageStructType)
)

func pushCumulativeUsageQuery(month time.Time) qtool.Template {
	table := fmt.Sprintf("usage/realtime/cumulative/%s-01/usage", month.Format("2006-01"))
	return qtool.Query(
		qtool.Declare("values", cumulativeUsageListType),
		qtool.ReplaceFromValues(table,
			"resource_id",
			"sku_id",
			"quantity",
		),
	)
}

func getAllCumulativeUsageQuery(month time.Time) qtool.Template {
	table := fmt.Sprintf("usage/realtime/cumulative/%s-01/usage", month.Format("2006-01"))
	return qtool.Query(
		"SELECT * FROM", qtool.Table(table),
	)
}

func (r CumulativeLogRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("resource_id", ydb.UTF8Value(r.ResourceID)),
		ydb.StructFieldValue("source_id", ydb.UTF8Value(r.SourceID)),
		ydb.StructFieldValue("sku_id", ydb.UTF8Value(r.SkuID)),
		ydb.StructFieldValue("first_use_month", ydb.UTF8Value(r.FirstUseMonth)),
		ydb.StructFieldValue("sequence_id", ydb.Uint64Value(r.SequenceID)),
		ydb.StructFieldValue("quantity", r.Quantity.Value()),
		ydb.StructFieldValue("delta", r.Delta.Value()),
	)
}

var (
	cumulativeLogsStructType = ydb.Struct(
		ydb.StructField("resource_id", ydb.TypeUTF8),
		ydb.StructField("source_id", ydb.TypeUTF8),
		ydb.StructField("sku_id", ydb.TypeUTF8),
		ydb.StructField("first_use_month", ydb.TypeUTF8),
		ydb.StructField("sequence_id", ydb.TypeUint64),
		ydb.StructField("quantity", ydb.Decimal(22, 9)),
		ydb.StructField("delta", ydb.Decimal(22, 9)),
	)
	cumulativeLogsListType = ydb.List(cumulativeLogsStructType)
)

func pushCumulativeLogsQuery(month time.Time) qtool.Template {
	table := fmt.Sprintf("usage/realtime/cumulative/%s-01/logs", month.Format("2006-01"))
	return qtool.Query(
		qtool.Declare("values", cumulativeLogsListType),
		qtool.ReplaceFromValues(table,
			"resource_id",
			"source_id",
			"sku_id",
			"first_use_month",
			"sequence_id",
			"quantity",
			"delta",
		),
	)
}

func getAllCumulativeLogsQuery(month time.Time) qtool.Template {
	table := fmt.Sprintf("usage/realtime/cumulative/%s-01/logs", month.Format("2006-01"))
	return qtool.Query(
		"SELECT * FROM", qtool.Table(table),
	)
}
