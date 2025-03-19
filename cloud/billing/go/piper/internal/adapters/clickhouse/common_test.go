package clickhouse

import (
	"context"
	"database/sql"
	"database/sql/driver"
	"errors"

	"github.com/DATA-DOG/go-sqlmock"
	"github.com/jmoiron/sqlx"
	"github.com/stretchr/testify/suite"
)

var errTest = errors.New("test error")

type clickhouseMockSuite struct {
	suite.Suite
	ctx  context.Context
	db   *sql.DB
	mock sqlmock.Sqlmock
}

func (suite *clickhouseMockSuite) SetupTest() {
	var err error
	suite.ctx = context.TODO()
	suite.db, suite.mock, err = sqlmock.New(sqlmock.ValueConverterOption(valueConverter{}))
	suite.Require().NoError(err)
}

func (suite *clickhouseMockSuite) dbx() *sqlx.DB {
	return sqlx.NewDb(suite.db, "clickhouse")
}

type valueConverter struct{}

func (valueConverter) ConvertValue(v interface{}) (driver.Value, error) {
	return v, nil
}

var (
	anyArg    = anyArgMatcher{}
	sqlResult = sqlmock.NewResult(0, 0)
)

type anyArgMatcher struct{}

func (anyArgMatcher) Match(_ driver.Value) bool {
	return true
}
