package meta

import (
	"context"
	"database/sql"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type conversionRatesTestSuite struct {
	baseSuite

	rows []ConversionRateRow
}

func TestConversionRates(t *testing.T) {
	suite.Run(t, new(conversionRatesTestSuite))
}

func (suite *conversionRatesTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()
	suite.rows = suite.rows[:0]
}

func (suite *conversionRatesTestSuite) TestTableCompliance() {
	err := suite.queries.pushConversionRates(suite.ctx, ConversionRateRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.GetAllConversionRates(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *conversionRatesTestSuite) TestGetAll() {
	suite.pushData()

	got, err := suite.queries.GetAllConversionRates(suite.ctx)
	suite.Require().NoError(err)
	suite.ElementsMatch(suite.rows, got)
}

func (suite *conversionRatesTestSuite) pushData() {
	suite.rows = []ConversionRateRow{
		{
			SourceCurrency: "RUB", TargetCurrency: "USD",
			EffectiveTime: qtool.UInt64Ts(time.Unix(0, 0)), Multiplier: qtool.DefaultDecimal(decimal.Must(decimal.FromString("0.012820513"))),
		},
		{
			SourceCurrency: "USD", TargetCurrency: "RUB",
			EffectiveTime: qtool.UInt64Ts(time.Unix(1, 0)), Multiplier: qtool.DefaultDecimal(decimal.Must(decimal.FromString("78"))),
		},
	}
	err := suite.queries.pushConversionRates(suite.ctx, suite.rows...)
	suite.Require().NoError(err)
}

func (q *Queries) pushConversionRates(ctx context.Context, rows ...ConversionRateRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushconversionRateQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (r ConversionRateRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("source_currency", ydb.UTF8Value(r.SourceCurrency)),
		ydb.StructFieldValue("target_currency", ydb.UTF8Value(r.TargetCurrency)),
		ydb.StructFieldValue("effective_time", r.EffectiveTime.Value()),
		ydb.StructFieldValue("multiplier", r.Multiplier.Value()),
	)
}

var (
	conversionRateStructType = ydb.Struct(
		ydb.StructField("source_currency", ydb.TypeUTF8),
		ydb.StructField("target_currency", ydb.TypeUTF8),
		ydb.StructField("effective_time", ydb.TypeUint64),
		ydb.StructField("multiplier", ydb.Decimal(22, 9)),
	)
	conversionRateListType = ydb.List(conversionRateStructType)

	pushconversionRateQuery = qtool.Query(
		qtool.Declare("values", conversionRateListType),
		qtool.ReplaceFromValues("utility/conversion_rates",
			"source_currency",
			"target_currency",
			"effective_time",
			"multiplier",
		),
	)
)
