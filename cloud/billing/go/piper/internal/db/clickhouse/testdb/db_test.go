package testdb

import (
	"context"
	"testing"
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"

	"github.com/jmoiron/sqlx"
	"github.com/stretchr/testify/suite"
)

type clickhouseSuite struct {
	suite.Suite

	ctx             context.Context
	cancel          context.CancelFunc
	connect         *sqlx.DB
	decimalProducer *decimal.ClickhouseProducer
}

func (suite *clickhouseSuite) SetupTest() {
	suite.ctx, suite.cancel = context.WithTimeout(context.Background(), time.Second*5)
	suite.connect = DB()

	converter, err := decimal.NewClickhouseProducer(38, 9)
	suite.Require().NoError(err)
	suite.decimalProducer = converter

	const ddl = `
		CREATE TABLE clickhouse_test_decimal128 (
		    	user_id Int64,
				decimal  Decimal(38, 9)
			) ENGINE Memory;
		`
	_, err = suite.connect.Exec(ddl)
	suite.Require().NoError(err)
}

func (suite *clickhouseSuite) TearDownTest() {
	defer suite.cancel()
	tx, err := suite.connect.Begin()
	suite.Require().NoError(err)

	stmt, err := suite.connect.Prepare("DROP TABLE clickhouse_test_decimal128")
	suite.Require().NoError(err)
	defer stmt.Close()

	_, err = stmt.Exec()
	suite.Require().NoError(err)

	err = tx.Commit()
	suite.Require().NoError(err)
}

func TestClickhouse(t *testing.T) {
	suite.Run(t, new(clickhouseSuite))
}

func (suite *clickhouseSuite) TestDecimal() {
	const (
		dml   = `INSERT INTO clickhouse_test_decimal128 (user_id,decimal) VALUES (:user_id, :decimal)`
		query = `SELECT decimal as value FROM clickhouse_test_decimal128 LIMIT 1`
	)

	expected := decimal.Must(decimal.FromString("5.123"))
	converted, err := suite.decimalProducer.Convert(expected)
	suite.Require().NoError(err)

	data := struct {
		UserID  int64  `db:"user_id"`
		Decimal []byte `db:"decimal"`
	}{
		UserID:  10,
		Decimal: converted,
	}

	// insert decimal value
	tx, err := suite.connect.Beginx()
	suite.Require().NoError(err)

	stmt, err := tx.PrepareNamed(dml)
	suite.Require().NoError(err)

	_, err = stmt.Exec(data)
	suite.Require().NoError(err)

	err = tx.Commit()
	suite.Require().NoError(err)

	// select decimal value
	var value []byte
	err = suite.connect.Get(&value, query)
	suite.Require().NoError(err)
	got, err := decimal.DecimalFromClickhouse(value, 38, 9)
	suite.Require().NoError(err)
	suite.Equal(got, expected)
}
