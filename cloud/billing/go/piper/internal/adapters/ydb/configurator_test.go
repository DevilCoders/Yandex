package ydb

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"
)

type configuratorTestSuite struct {
	dbMockSuite

	ctx          context.Context
	ctxCancel    context.CancelFunc
	configurator *Configurator
}

func TestConfigurator(t *testing.T) {
	suite.Run(t, new(configuratorTestSuite))
}

func (suite *configuratorTestSuite) SetupTest() {
	suite.dbMockSuite.SetupTest()

	suite.ctx, suite.ctxCancel = context.WithCancel(context.Background())
	suite.configurator = NewConfigurator(suite.ctx, suite.dbx(), "")
}

func (suite *configuratorTestSuite) TearDownTest() {
	suite.ctxCancel()
}

func (suite *configuratorTestSuite) TestGetConfig() {
	contextResult := suite.mock.NewRows([]string{"key", "value"}).
		AddRow("ns.key1", "value1").
		AddRow("ns.key2", "value2")
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT.*FROM `utility/context`").WillReturnRows(contextResult).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.configurator.GetConfig(suite.ctx, "ns")
	want := map[string]string{
		"key1": "value1",
		"key2": "value2",
	}

	suite.Require().NoError(err)
	suite.Require().EqualValues(want, got)

	suite.Require().NoError(suite.mock.ExpectationsWereMet())
}

func (suite *configuratorTestSuite) TestGetConfigDbErr() {
	suite.mock.ExpectBegin().WillReturnError(errTest)
	_, err := suite.configurator.GetConfig(suite.ctx, "ns")
	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
}
