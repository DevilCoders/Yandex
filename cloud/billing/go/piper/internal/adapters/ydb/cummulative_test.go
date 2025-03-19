package ydb

import (
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type cumulativeTestSuite struct {
	baseMetaSessionTestSuite

	month time.Time
}

func TestCumulative(t *testing.T) {
	suite.Run(t, new(cumulativeTestSuite))
}

func (suite *cumulativeTestSuite) SetupTest() {
	suite.baseMetaSessionTestSuite.SetupTest()
	suite.schemeMock.On("ExecuteSchemeQuery", mock.Anything, mock.Anything).Return(nil)
	suite.month = time.Date(2000, 07, 31, 0, 0, 0, 0, time.Local)
}

func (suite *cumulativeTestSuite) TestCalculate() {
	suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").
		WillReturnRows(suite.mock.NewRows([]string{"cnt"}).AddRow(3)).
		RowsWillBeClosed()

	logRows := suite.mock.NewRows([]string{
		"resource_id", "source_id", "sku_id", "first_use_month", "sequence_id", "quantity", "delta",
	}).
		AddRow("res", "src", "sku", "2000-07-01", uint64(10), dbDecimal("100"), dbDecimal("50")).
		AddRow("res2", "src", "sku", "2000-06-01", uint64(10), dbDecimal("100"), dbDecimal("50"))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").WillReturnRows(logRows).RowsWillBeClosed()

	cnt, log, err := suite.session.CalculateCumulativeUsage(
		suite.ctx, entities.ProcessingScope{MinMessageOffset: 1, MaxMessageOffset: 100, SourceID: "src-lb:10"},
		entities.UsagePeriod{Period: suite.month, PeriodType: entities.MonthlyUsage}, []entities.CumulativeSource{
			{ResourceID: "res", SkuID: "sku", PricingQuantity: decimal.Decimal128{}, MetricOffset: 10},
			{ResourceID: "res2", SkuID: "sku", PricingQuantity: decimal.Decimal128{}, MetricOffset: 10},
		},
	)

	suite.Require().NoError(err)
	suite.EqualValues(3, cnt)

	delta := decimal.Must(decimal.FromInt64(50))
	wantLog := []entities.CumulativeUsageLog{
		{FirstPeriod: true, ResourceID: "res", SkuID: "sku", Delta: delta, MetricOffset: 10},
		{FirstPeriod: false, ResourceID: "res2", SkuID: "sku", Delta: delta, MetricOffset: 10},
	}
	suite.ElementsMatch(wantLog, log)

	suite.NoError(suite.mock.ExpectationsWereMet())
}

func (suite *cumulativeTestSuite) TestCalculatePaginated() {
	suite.session.batchSizeOverride = 2

	suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").
		WillReturnRows(suite.mock.NewRows([]string{"cnt"}).AddRow(3)).
		RowsWillBeClosed()

	logRows1 := suite.mock.NewRows([]string{
		"resource_id", "source_id", "sku_id", "first_use_month", "sequence_id", "quantity", "delta",
	}).
		AddRow("res", "src", "sku", "2000-07-01", uint64(10), dbDecimal("100"), dbDecimal("50")).
		AddRow("res2", "src", "sku", "2000-06-01", uint64(10), dbDecimal("100"), dbDecimal("50"))
	logRows2 := suite.mock.NewRows([]string{
		"resource_id", "source_id", "sku_id", "first_use_month", "sequence_id", "quantity", "delta",
	}).
		AddRow("res3", "src", "sku", "2000-07-01", uint64(10), dbDecimal("100"), dbDecimal("50"))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").WillReturnRows(logRows1).RowsWillBeClosed()
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").WillReturnRows(logRows2).RowsWillBeClosed()

	cnt, log, err := suite.session.CalculateCumulativeUsage(
		suite.ctx, entities.ProcessingScope{MinMessageOffset: 1, MaxMessageOffset: 100, SourceID: "src-lb:10"},
		entities.UsagePeriod{Period: suite.month, PeriodType: entities.MonthlyUsage}, []entities.CumulativeSource{
			{ResourceID: "res", SkuID: "sku", PricingQuantity: decimal.Decimal128{}, MetricOffset: 10},
			{ResourceID: "res2", SkuID: "sku", PricingQuantity: decimal.Decimal128{}, MetricOffset: 10},
			{ResourceID: "res3", SkuID: "sku", PricingQuantity: decimal.Decimal128{}, MetricOffset: 10},
		},
	)

	suite.Require().NoError(err)
	suite.EqualValues(3, cnt)

	delta := decimal.Must(decimal.FromInt64(50))
	wantLog := []entities.CumulativeUsageLog{
		{FirstPeriod: true, ResourceID: "res", SkuID: "sku", Delta: delta, MetricOffset: 10},
		{FirstPeriod: false, ResourceID: "res2", SkuID: "sku", Delta: delta, MetricOffset: 10},
		{FirstPeriod: true, ResourceID: "res3", SkuID: "sku", Delta: delta, MetricOffset: 10},
	}
	suite.ElementsMatch(wantLog, log)

	suite.NoError(suite.mock.ExpectationsWereMet())
}

func (suite *cumulativeTestSuite) TestPeriodUnknown() {
	_, _, err := suite.session.CalculateCumulativeUsage(
		suite.ctx, entities.ProcessingScope{}, entities.UsagePeriod{}, []entities.CumulativeSource{},
	)

	suite.Require().Error(err)
	suite.ErrorIs(err, ErrCumulativePeriodType)
}

func (suite *cumulativeTestSuite) TestCalculationError() {
	suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").WillReturnError(errTest)
	_, _, err := suite.session.CalculateCumulativeUsage(
		suite.ctx, entities.ProcessingScope{MinMessageOffset: 1, MaxMessageOffset: 100, SourceID: "src-lb:10"},
		entities.UsagePeriod{Period: suite.month, PeriodType: entities.MonthlyUsage}, []entities.CumulativeSource{
			{ResourceID: "res", SkuID: "sku", PricingQuantity: decimal.Decimal128{}, MetricOffset: 10},
		},
	)

	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
}

func (suite *cumulativeTestSuite) TestLogFetchTxError() {
	suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").
		WillReturnRows(suite.mock.NewRows([]string{"cnt"}).AddRow(3)).
		RowsWillBeClosed()
	suite.mock.ExpectBegin().WillReturnError(errTest)

	_, _, err := suite.session.CalculateCumulativeUsage(
		suite.ctx, entities.ProcessingScope{MinMessageOffset: 1, MaxMessageOffset: 100, SourceID: "src-lb:10"},
		entities.UsagePeriod{Period: suite.month, PeriodType: entities.MonthlyUsage}, []entities.CumulativeSource{
			{ResourceID: "res", SkuID: "sku", PricingQuantity: decimal.Decimal128{}, MetricOffset: 10},
		},
	)

	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
}

func (suite *cumulativeTestSuite) TestLogCreateOnce() {
	adapter := suite.session.adapter
	logRows := suite.mock.NewRows([]string{
		"resource_id", "source_id", "sku_id", "first_use_month", "sequence_id", "quantity", "delta",
	})

	for i := 0; i < 10; i++ {
		sess := adapter.Session() // run from different sessions. create tables once per process
		suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").
			WillReturnRows(suite.mock.NewRows([]string{"cnt"}).AddRow(0)).
			RowsWillBeClosed()
		suite.mock.ExpectBegin()
		suite.mock.ExpectQuery("`usage/realtime/cumulative/2000-07-01/logs`").WillReturnRows(logRows).RowsWillBeClosed()

		_, _, err := sess.CalculateCumulativeUsage(
			suite.ctx, entities.ProcessingScope{MinMessageOffset: 1, MaxMessageOffset: 100, SourceID: "src-lb:10"},
			entities.UsagePeriod{Period: suite.month, PeriodType: entities.MonthlyUsage}, []entities.CumulativeSource{
				{ResourceID: "res", SkuID: "sku", PricingQuantity: decimal.Decimal128{}, MetricOffset: 10},
			},
		)
		suite.Require().NoError(err)
	}

	// We want only 3 calls: prev, cur and next months
	suite.schemeMock.AssertNumberOfCalls(suite.T(), "ExecuteSchemeQuery", 3)
}
