package utility

import (
	"context"
	"database/sql"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type contextTestSuite struct {
	baseSuite

	rows []fullContextRow
}

func TestContext(t *testing.T) {
	suite.Run(t, new(contextTestSuite))
}

func (suite *contextTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()
	suite.rows = suite.rows[:0]
}

func (suite *contextTestSuite) TestTableCompliance() {
	err := suite.queries.pushSchemas(suite.ctx, fullContextRow{Key: "key.subkey"})
	suite.Require().NoError(err)

	_, err = suite.queries.GetContext(suite.ctx, "key")
	suite.Require().NoError(err)
}

func (suite *contextTestSuite) TestGetByNamespace() {
	suite.pushData()

	want := make([]ContextRow, 0)
	for _, r := range suite.rows[:2] {
		want = append(want, ContextRow{Key: r.Key, Value: r.Value})
	}
	got, err := suite.queries.GetContext(suite.ctx, "ns1.")
	suite.Require().NoError(err)
	suite.ElementsMatch(want, got)
}

func (suite *contextTestSuite) TestGetAll() {
	suite.pushData()

	want := make([]ContextRow, 0)
	for _, r := range suite.rows {
		want = append(want, ContextRow{Key: r.Key, Value: r.Value})
	}
	got, err := suite.queries.GetContext(suite.ctx, "")
	suite.Require().NoError(err)
	suite.ElementsMatch(want, got)
}

func (suite *contextTestSuite) pushData() {
	suite.rows = []fullContextRow{
		{Key: "ns1.key1", Value: "value1", CreatedAt: qtool.UInt64Ts(time.Unix(0, 0)), CreatedBy: "usr1"},
		{Key: "ns1.key2", Value: "value2", CreatedAt: qtool.UInt64Ts(time.Unix(1, 0)), CreatedBy: "usr2"},
		{Key: "ns2.key1", Value: "value1", CreatedAt: qtool.UInt64Ts(time.Unix(2, 0)), CreatedBy: "usr1"},
	}
	err := suite.queries.pushSchemas(suite.ctx, suite.rows...)
	suite.Require().NoError(err)
}

func (q *Queries) pushSchemas(ctx context.Context, rows ...fullContextRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, contextSchemaQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

type fullContextRow struct {
	Key       string         `db:"key"`
	Value     string         `db:"value"`
	CreatedAt qtool.UInt64Ts `db:"created_at"`
	CreatedBy string         `db:"created_by"`
}

func (r fullContextRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("key", ydb.UTF8Value(r.Key)),
		ydb.StructFieldValue("value", ydb.UTF8Value(r.Value)),
		ydb.StructFieldValue("created_at", r.CreatedAt.Value()),
		ydb.StructFieldValue("created_by", ydb.UTF8Value(r.CreatedBy)),
	)
}

var (
	contextStructType = ydb.Struct(
		ydb.StructField("key", ydb.TypeUTF8),
		ydb.StructField("value", ydb.TypeUTF8),
		ydb.StructField("created_at", ydb.TypeUint64),
		ydb.StructField("created_by", ydb.TypeUTF8),
	)
	contextListType = ydb.List(contextStructType)

	contextSchemaQuery = qtool.Query(
		qtool.Declare("values", contextListType),
		qtool.ReplaceFromValues("utility/context", "key", "value", "created_at", "created_by"),
	)
)
