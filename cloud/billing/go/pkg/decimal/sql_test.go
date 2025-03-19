package decimal

import (
	"database/sql"
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"
)

var (
	_ sql.Scanner = &DBDecimal{}
	_ sql.Scanner = &DBNullDecimal{}
)

type sqlSuite struct {
	suite.Suite
}

func TestSQL(t *testing.T) {
	suite.Run(t, new(sqlSuite))
}

func (suite *sqlSuite) TestScan() {
	for i, v := range []interface{}{
		float32(1), float64(1), int64(1), `"1"`, []byte{'1'},
	} {
		suite.Run(fmt.Sprintf("case-%d", i), func() {

			var d DBDecimal
			err := d.Scan(v)
			suite.Require().NoError(err)
			suite.Equal("1", d.String())
		})
	}
}

func (suite *sqlSuite) TestScanError() {
	for i, v := range []interface{}{
		int32(1), "x", nil,
	} {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			var d DBDecimal
			err := d.Scan(v)
			suite.Require().Error(err)
		})
	}
}

func (suite *sqlSuite) TestScanNull() {
	var d DBNullDecimal
	err := d.Scan(nil)
	suite.Require().NoError(err)
	suite.False(d.Valid)
}

func (suite *sqlSuite) TestScanNullValue() {
	var d DBNullDecimal
	err := d.Scan("1")
	suite.Require().NoError(err)
	suite.True(d.Valid)
}

func (suite *sqlSuite) TestValue() {
	d := DBDecimal{Must(FromInt64(1))}
	v, err := d.Value()
	suite.Require().NoError(err)
	suite.EqualValues("1", v)
}

func (suite *sqlSuite) TestValueNull() {
	d := DBNullDecimal{}
	v, err := d.Value()
	suite.Require().NoError(err)
	suite.Nil(v)
}

func (suite *sqlSuite) TestValueNullable() {
	d := DBNullDecimal{Valid: true}
	d.Decimal128 = Must(FromInt64(1))
	v, err := d.Value()
	suite.Require().NoError(err)
	suite.EqualValues("1", v)
}
