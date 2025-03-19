package meta

import (
	"context"
	"database/sql"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type schemasTestSuite struct {
	baseSuite

	rows []SchemaRow
}

func TestSchemas(t *testing.T) {
	suite.Run(t, new(schemasTestSuite))
}

func (suite *schemasTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()
	suite.rows = suite.rows[:0]
}

func (suite *schemasTestSuite) TestTableCompliance() {
	err := suite.queries.pushSchemas(suite.ctx, SchemaRow{Name: "sch"})
	suite.Require().NoError(err)

	_, err = suite.queries.GetSchemasByName(suite.ctx, "sch")
	suite.Require().NoError(err)
}

func (suite *schemasTestSuite) TestGetByName() {
	suite.pushData()

	got, err := suite.queries.GetSchemasByName(suite.ctx, "schema2", "schema3", "no_schema")
	suite.Require().NoError(err)
	suite.ElementsMatch(suite.rows[1:], got)
}

func (suite *schemasTestSuite) TestGetAll() {
	suite.pushData()

	got, err := suite.queries.GetAllSchemas(suite.ctx)
	suite.Require().NoError(err)
	suite.ElementsMatch(suite.rows, got)
}

func (suite *schemasTestSuite) pushData() {
	suite.rows = []SchemaRow{
		{Name: "schema1", ServiceID: "service1", Tags: qtool.JSONAnything("1")},
		{Name: "schema2", ServiceID: "service2", Tags: qtool.JSONAnything("2")},
		{Name: "schema3", ServiceID: "service3", Tags: qtool.JSONAnything("null")},
	}
	err := suite.queries.pushSchemas(suite.ctx, suite.rows...)
	suite.Require().NoError(err)
}

func (q *Queries) pushSchemas(ctx context.Context, rows ...SchemaRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushSchemaQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (r SchemaRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("service_id", ydb.UTF8Value(r.ServiceID)),
		ydb.StructFieldValue("name", ydb.UTF8Value(r.Name)),
		ydb.StructFieldValue("tags", r.Tags.Value()),
	)
}

var (
	schemaStructType = ydb.Struct(
		ydb.StructField("service_id", ydb.TypeUTF8),
		ydb.StructField("name", ydb.TypeUTF8),
		ydb.StructField("tags", ydb.Optional(ydb.TypeJSON)),
	)
	schemaListType = ydb.List(schemaStructType)

	pushSchemaQuery = qtool.Query(
		qtool.Declare("values", schemaListType),
		qtool.ReplaceFromValues("meta/schemas", "service_id", "name", "tags"),
	)
)
