package ydb

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"
)

type adapterTestSuite struct {
	dbMockSuite
}

func TestAdapter(t *testing.T) {
	suite.Run(t, new(adapterTestSuite))
}

func (suite *adapterTestSuite) TestNewAdapter() {
	ctx := context.Background()
	a, err := NewMetaAdapter(ctx, suite.dbx(), suite.schemeMock, "", "")
	suite.Require().NoError(err)
	suite.Require().NotNil(a)
}

func (suite *adapterTestSuite) TestSession() {
	ctx := context.Background()
	a, err := NewMetaAdapter(ctx, suite.dbx(), suite.schemeMock, "", "")
	suite.Require().NoError(err)

	session := a.Session()
	suite.Require().NotNil(session)
	suite.Equal(a, session.adapter)
}

type baseSessionTestSuite struct {
	dbMockSuite

	ctx       context.Context
	ctxCancel context.CancelFunc
	session   *MetaSession
}

func (suite *baseSessionTestSuite) SetupTest() {
	suite.dbMockSuite.SetupTest()

	suite.ctx, suite.ctxCancel = context.WithCancel(context.Background())
	a, err := NewMetaAdapter(suite.ctx, suite.dbx(), suite.schemeMock, "", "")
	suite.Require().NoError(err)

	suite.session = a.Session()
}

func (suite *baseSessionTestSuite) TearDownTest() {
	suite.ctxCancel()
}

type baseMetaSessionTestSuite struct {
	baseSessionTestSuite
	session *MetaSession
}

func (suite *baseMetaSessionTestSuite) SetupTest() {
	suite.baseSessionTestSuite.SetupTest()

	a, err := NewMetaAdapter(suite.ctx, suite.dbx(), suite.schemeMock, "", "")
	suite.Require().NoError(err)

	suite.session = a.Session()
}
