package ydb

import (
	"context"
	"database/sql"
	"database/sql/driver"
	"errors"

	"github.com/DATA-DOG/go-sqlmock"
	"github.com/jmoiron/sqlx"
	"github.com/stretchr/testify/suite"

	ydec "a.yandex-team.ru/kikimr/public/sdk/go/ydb/decimal"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

var errTest = errors.New("test error")

type dbMockSuite struct {
	suite.Suite
	db         *sql.DB
	mock       sqlmock.Sqlmock
	schemeMock *MockSchemeSession
}

func (suite *dbMockSuite) SetupTest() {
	var err error
	suite.db, suite.mock, err = sqlmock.New(sqlmock.ValueConverterOption(valueConverter{}))
	suite.Require().NoError(err)

	suite.schemeMock = &MockSchemeSession{}
}

func (suite *dbMockSuite) dbx() *sqlx.DB {
	return sqlx.NewDb(suite.db, "ydb")
}

type valueConverter struct{}

func (valueConverter) ConvertValue(v interface{}) (driver.Value, error) {
	return v, nil
}

func dbDecimal(value string) ydbsql.Decimal {
	v, _ := ydec.Parse(value, 22, 9)
	return ydbsql.Decimal{
		Bytes:     ydec.Int128(v, 22, 9),
		Precision: 22,
		Scale:     9,
	}
}

var (
	anyArg    = anyArgMatcher{}
	sqlResult = sqlmock.NewResult(0, 0) // ydb does not supported result report
)

type anyArgMatcher struct{}

func (anyArgMatcher) Match(_ driver.Value) bool { // YBD params can not be matched simple way
	return true
}

func (_m *MockSchemeSession) Get(context.Context) (SchemeSession, error) {
	return _m, nil
}

func (_m *MockSchemeSession) Put(context.Context, SchemeSession) error {
	return nil
}
