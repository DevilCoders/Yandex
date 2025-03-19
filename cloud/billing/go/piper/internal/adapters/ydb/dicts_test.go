package ydb

import (
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type schemaTestSuite struct {
	baseMetaSessionTestSuite
}

func TestSchema(t *testing.T) {
	suite.Run(t, new(schemaTestSuite))
}

func (suite *schemaTestSuite) TestSchemaGet() {
	schemasResult := suite.mock.NewRows([]string{"service_id", "name", "tags"}).
		AddRow("", "test.schema", `{"required":["tag1", "tag2"],"optional":["tag3", "tag4"]}`).
		AddRow("", "other.schema", `{"required":["otherTag1", "otherTag2"],"optional":["otherTag3", "otherTag4"]}`).
		AddRow("", "other.schema", nil)
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT.*FROM `meta/schemas`").WillReturnRows(schemasResult).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	result, err := suite.session.GetMetricSchema(suite.ctx, entities.ProcessingScope{}, "test.schema")
	suite.Require().NoError(err)

	want := entities.MetricsSchema{
		Schema:       "test.schema",
		RequiredTags: []string{"tag1", "tag2"},
	}

	suite.EqualValues(want, result)
	suite.Require().NoError(suite.mock.ExpectationsWereMet())
}

func (suite *schemaTestSuite) TestSchemaGetFromCache() {
	// Simulating filled adapter's cache. No queries should be executed.
	suite.session.adapter.schemas.namesIndex = map[string]int{"test.schema": 0}
	suite.session.adapter.schemas.tags = []schemaTags{{Required: []string{"tag1", "tag2"}}}
	suite.session.adapter.schemas.updated()

	result, err := suite.session.GetMetricSchema(suite.ctx, entities.ProcessingScope{}, "test.schema")
	suite.Require().NoError(err)
	suite.Require().ElementsMatch([]string{"tag1", "tag2"}, result.RequiredTags)

	// Session should use state collected during first run.
	suite.session.adapter.schemas.tags = []schemaTags{{Required: []string{"tag3"}}}
	result, err = suite.session.GetMetricSchema(suite.ctx, entities.ProcessingScope{}, "test.schema")
	suite.Require().NoError(err)
	suite.Require().ElementsMatch([]string{"tag1", "tag2"}, result.RequiredTags)
}

func (suite *schemaTestSuite) TestSchemasExpired() {
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT.*FROM `meta/schemas`").WillReturnError(errTest)
	suite.mock.ExpectRollback()

	_, err := suite.session.GetMetricSchema(suite.ctx, entities.ProcessingScope{}, "test.schema")
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrSchemasExpired)
	suite.ErrorIs(err, errTest)
}

func (suite *schemaTestSuite) TestSchemasRefreshError() {
	suite.session.adapter.schemas.namesIndex = map[string]int{"test.schema": 0}
	suite.session.adapter.schemas.tags = []schemaTags{{Required: []string{"tag1", "tag2"}}}
	suite.session.adapter.schemas.updatedAt = time.Now().Add(-storesRefreshRate - time.Second)

	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT.*FROM `meta/schemas`").WillReturnError(errTest)

	result, err := suite.session.GetMetricSchema(suite.ctx, entities.ProcessingScope{}, "test.schema")
	suite.Require().NoError(err)
	suite.Require().ElementsMatch([]string{"tag1", "tag2"}, result.RequiredTags)
}

func (suite *schemaTestSuite) TestSchemaNotFound() {
	schemasResult := suite.mock.NewRows([]string{"service_id", "name", "tags"}).
		AddRow("", "other.schema", nil)
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT.*FROM `meta/schemas`").WillReturnRows(schemasResult).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	schema, err := suite.session.GetMetricSchema(suite.ctx, entities.ProcessingScope{}, "test.schema")
	suite.Require().NoError(err)
	suite.Require().True(schema.Empty())
}

func (suite *schemaTestSuite) TestSchemaDataError() {
	schemasResult := suite.mock.NewRows([]string{"service_id", "name", "tags"}).
		AddRow("", "other.schema", "ERROR")
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT.*FROM `meta/schemas`").WillReturnRows(schemasResult).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	_, err := suite.session.GetMetricSchema(suite.ctx, entities.ProcessingScope{}, "test.schema")
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrSchemaData)
}

type unitsTestSuite struct {
	baseMetaSessionTestSuite
}

func TestUnits(t *testing.T) {
	suite.Run(t, new(unitsTestSuite))
}

func (suite *unitsTestSuite) TestUnitsRefresh() {
	suite.mockData()
	qnt, _ := decimal.FromInt64(100)

	_, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, time.Now(), "su", "du", qnt)
	suite.Require().NoError(err)
	suite.Require().NoError(suite.mock.ExpectationsWereMet())
}

func (suite *unitsTestSuite) TestConvertDefault() {
	suite.mockData()
	qnt, _ := decimal.FromInt64(100)
	want, _ := decimal.FromInt64(50)

	got, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, time.Now(), "su", "du", qnt)
	suite.Require().NoError(err)

	suite.Zero(want.Cmp(got), got.String())
}

func (suite *unitsTestSuite) TestConvertSameUnit() {
	suite.mockData()
	qnt, _ := decimal.FromInt64(100)
	want, _ := decimal.FromInt64(100)

	got, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, time.Now(), "u", "u", qnt)
	suite.Require().NoError(err)

	suite.Zero(want.Cmp(got), got.String())
}

func (suite *unitsTestSuite) TestConvertSameUnknownUnit() {
	suite.mockData()
	qnt, _ := decimal.FromInt64(100)
	want, _ := decimal.FromInt64(100)

	got, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, time.Now(), "unkn", "unkn", qnt)
	suite.Require().NoError(err)

	suite.Zero(want.Cmp(got), got.String())
}

func (suite *unitsTestSuite) TestConvertReverse() {
	suite.mockData()
	qnt, _ := decimal.FromInt64(100)
	want, _ := decimal.FromInt64(200)

	got, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, time.Now(), "rsu", "rdu", qnt)
	suite.Require().NoError(err)

	suite.Zero(want.Cmp(got), got.String())
}

func (suite *unitsTestSuite) TestConvertMonth() {
	suite.mockData()
	qnt, _ := decimal.FromInt64(100)
	febDays, _ := decimal.FromInt64(28)
	monthDays, _ := decimal.FromInt64(30)
	want := decimal.Must(decimal.FromInt64(50)).Mul(monthDays.Div(febDays))
	at := time.Date(2021, time.February, 1, 23, 59, 59, 0, timetool.DefaultTz())

	got, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, at, "msu", "mdu", qnt)
	suite.Require().NoError(err)

	suite.Zero(want.Cmp(got), "%f!=%f", want, got)
}

func (suite *unitsTestSuite) TestConvertCurrency() {
	suite.mockData()
	qnt, _ := decimal.FromInt64(100)
	want, _ := decimal.FromInt64(10000)
	at := time.Now()

	got, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, at, "kop", "cent", qnt)
	suite.Require().NoError(err)
	suite.Zero(want.Cmp(got), got.String())

	at = time.Unix(1000, 0)
	got, err = suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, at, "kop", "cent", qnt)
	suite.Require().NoError(err)
	suite.Zero(want.Cmp(got), got.String())

	at = time.Unix(999, 0)
	want, _ = decimal.FromInt64(5000)
	got, err = suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, at, "kop", "cent", qnt)
	suite.Require().NoError(err)
	suite.Zero(want.Cmp(got), got.String())
}

func (suite *unitsTestSuite) TestConvertRuleTypeError() {
	suite.mockData()
	qnt, _ := decimal.FromInt64(100)

	_, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, time.Now(), "incs", "incd", qnt)
	suite.Require().Error(err)
	suite.Require().ErrorIs(err, ErrUnitsConversionRule)
}

func (suite *unitsTestSuite) TestConvertNoRule() {
	suite.mockData()
	suite.mockData() // we will refresh cache from db
	qnt, _ := decimal.FromInt64(100)

	_, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, time.Now(), "su", "unkn", qnt)
	suite.Require().Error(err)
	suite.Require().ErrorIs(err, ErrUnitRuleNotFound)
}

func (suite *unitsTestSuite) TestUnitsExpired() {
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT.*FROM `meta/units`").WillReturnError(errTest)
	suite.mock.ExpectRollback()

	qnt, _ := decimal.FromInt64(100)

	_, err := suite.session.ConvertQuantity(suite.ctx, entities.ProcessingScope{}, time.Now(), "su", "unkn", qnt)
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrUnitsExpired)
	suite.ErrorIs(err, errTest)
}

func (suite *unitsTestSuite) mockData() {
	unitsResult := suite.mock.NewRows([]string{"src_unit", "dst_unit", "factor", "reverse", "type"}).
		AddRow("su", "du", uint64(2), false, defaultConversionRule).
		AddRow("u", "u", uint64(1), false, defaultConversionRule).
		AddRow("rsu", "rdu", uint64(2), true, defaultConversionRule).
		AddRow("msu", "mdu", uint64(2), false, monthConversionRule).
		AddRow("kop", "cent", uint64(1), false, multiCurrencyConversionRule).
		AddRow("incs", "incd", uint64(1), false, "bad type")
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT.*FROM `meta/units`").WillReturnRows(unitsResult).RowsWillBeClosed()

	ratesResult := suite.mock.NewRows([]string{"source_currency", "target_currency", "effective_time", "multiplier"}).
		AddRow("rub", "usd", uint64(0), dbDecimal("50")).
		AddRow("usd", "rub", uint64(0), dbDecimal("0.02")).
		AddRow("rub", "usd", uint64(1000), dbDecimal("100")).
		AddRow("usd", "rub", uint64(1000), dbDecimal("0.01"))
	suite.mock.ExpectQuery("SELECT.*FROM `utility/conversion_rates`").WillReturnRows(ratesResult).RowsWillBeClosed()
	suite.mock.ExpectCommit()
}
