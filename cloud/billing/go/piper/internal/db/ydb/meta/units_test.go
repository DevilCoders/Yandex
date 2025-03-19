package meta

import (
	"context"
	"database/sql"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type unitsTestSuite struct {
	baseSuite

	rows []UnitRow
}

func TestUnits(t *testing.T) {
	suite.Run(t, new(unitsTestSuite))
}

func (suite *unitsTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()
	suite.rows = suite.rows[:0]
}

func (suite *unitsTestSuite) TestTableCompliance() {
	err := suite.queries.pushUnits(suite.ctx, UnitRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.GetAllUnits(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *unitsTestSuite) TestGetAll() {
	suite.pushData()

	got, err := suite.queries.GetAllUnits(suite.ctx)
	suite.Require().NoError(err)
	suite.ElementsMatch(suite.rows, got)
}

func (suite *unitsTestSuite) pushData() {
	suite.rows = []UnitRow{
		{SrcUnit: "src1", DstUnit: "dst1", Factor: 1, Reverse: false, Type: "default"},
		{SrcUnit: "src2", DstUnit: "dst2", Factor: 10, Reverse: true, Type: "calendar_month"},
		{SrcUnit: "src3", DstUnit: "dst3", Factor: 20, Reverse: false, Type: "multi_currency"},
	}
	err := suite.queries.pushUnits(suite.ctx, suite.rows...)
	suite.Require().NoError(err)
}

func (q *Queries) pushUnits(ctx context.Context, rows ...UnitRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushUnitQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (r UnitRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("src_unit", ydb.UTF8Value(r.SrcUnit)),
		ydb.StructFieldValue("dst_unit", ydb.UTF8Value(r.DstUnit)),
		ydb.StructFieldValue("factor", ydb.Uint64Value(r.Factor)),
		ydb.StructFieldValue("reverse", ydb.BoolValue(r.Reverse)),
		ydb.StructFieldValue("type", ydb.UTF8Value(r.Type)),
	)
}

var (
	unitStructType = ydb.Struct(
		ydb.StructField("src_unit", ydb.TypeUTF8),
		ydb.StructField("dst_unit", ydb.TypeUTF8),
		ydb.StructField("factor", ydb.TypeUint64),
		ydb.StructField("reverse", ydb.TypeBool),
		ydb.StructField("type", ydb.TypeUTF8),
	)
	unitListType = ydb.List(unitStructType)

	pushUnitQuery = qtool.Query(
		qtool.Declare("values", unitListType),
		qtool.ReplaceFromValues("meta/units",
			"src_unit",
			"dst_unit",
			"factor",
			"reverse",
			"type",
		),
	)
)
